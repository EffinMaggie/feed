/*
 * This file is part of the ef.gy project.
 * See the appropriate repository at http://ef.gy/.git for exact file
 * modification records.
*/

/*
 * Copyright (c) 2013, Magnus Deininger
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
*/

#if !defined(FEED_ICAL_H)
#define FEED_ICAL_H

#include <feed/handler.h>
#include <feed/entry.h>
#include <feed/xml.h>
#include <feed/download.h>
#include <boost/regex.hpp>
#include <sstream>
#include <algorithm>
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp>

namespace feed
{
    class ical : public handler
    {
    protected:
        sqlite::statement selectBlock;
        sqlite::statement insertBlock;
        sqlite::statement insertProperty;
        sqlite::statement insertAttribute;

        class block
        {
        public:
            block(ical &pContext)
                : context(pContext),
                  uid(),
                  type(), key(), value(), attributes(), autocommit(false)
            {}

            block(const block &b)
                : context(b.context),
                  uid(b.uid),
                  type(b.type), key(b.key), value(b.value), attributes(b.attributes), autocommit(false)
            {}

            ~block(void)
            {
                try
                {
                    autocommit && commit();
                }
                catch (exception &e)
                {
                    std::cerr << "ical::block::~block: EXCEPTION: " << e.string << "\n";
                }
            }

            block &operator = (block &b)
            {
                uid        = b.uid;
                type       = b.type;
                key        = b.key;
                value      = b.value;
                attributes = b.attributes;
                b.autocommit = false;
                return *this;
            }

            bool commit (void)
            {
                if (uid != "")
                {
                    int ibid = -1;

                    std::cerr << "commit: type=" << type << ": " << uid << "\n";
                    context.selectBlock.bind(1, uid);
                    if (context.selectBlock.step() && context.selectBlock.row)
                    {
                        context.selectBlock.get(0, ibid);
                    }
                    context.selectBlock.reset();

                    if (ibid < 0)
                    {
                        context.insertBlock.bind(1, uid);
                        context.insertBlock.bind(2, type);
                        context.insertBlock.step();
                        ibid = sqlite3_last_insert_rowid(context.context.sql);
                        context.insertBlock.reset();
                    }

                    context.insertProperty.bind(1, ibid);

                    std::vector<std::string>::iterator ik = key.begin();
                    std::vector<std::string>::iterator iv = value.begin();
                    std::vector<std::map<std::string,std::vector<std::string> > >::iterator ia = attributes.begin();

                    while ((ik != key.end()) && (iv != value.end()) && (ia != attributes.end()))
                    {
                        context.insertProperty.bind(2, *ik);
                        context.insertProperty.bind(3, *iv);
                        context.insertProperty.step();
                        int ipid = sqlite3_last_insert_rowid(context.context.sql);
                        context.insertProperty.reset();

                        context.insertAttribute.bind(1, ipid);
                        for (std::map<std::string,std::vector<std::string> >::iterator it = ia->begin();
                             it != ia->end();
                             it++)
                        {
                            context.insertAttribute.bind(2, it->first);
                            for (std::vector<std::string>::iterator it2 = it->second.begin();
                                 it2 != it->second.end();
                                 it2++)
                            {
                                context.insertAttribute.bind(3, *it2);
                                context.insertAttribute.stepReset();
                            }
                        }

                        ik++;
                        iv++;
                        ia++;
                    }
                }

                return true;
            }

            std::string uid;
            std::string type;
            std::vector<std::string> key;
            std::vector<std::string> value;
            std::vector<std::map<std::string,std::vector<std::string> > > attributes;
            bool autocommit;

        protected:
            ical &context;
        };

    public:
        ical(configuration &pConfiguration, const bool &pEnabled, download &pDownload)
            : handler(pConfiguration, stICal, pEnabled), download(pDownload),
              selectBlock("select id from icalblock where uid=?1", context.sql),
              insertBlock("insert or replace into icalblock (uid, type) values (?1, ?2)", context.sql),
              insertProperty("insert or replace into icalproperty (ibid, key, value) values (?1, ?2, ?3)", context.sql),
              insertAttribute("insert or replace into icalattribute (ipid, key, value) values (?1, ?2, ?3)", context.sql)
        {}

        virtual bool handle
            (const enum servicetype &st,
             feed::feed &feed)
        {
            if (!enabled)
            {
                return false;
            }

            if (st != stICal)
            {
                return false;
            }
            std::cerr << "I";

            try
            {
                download::data data = download.retrieve (feed.source);
                static const boost::regex folded("^\\s(.*)");
                std::istringstream is(data.content);

                std::vector<std::string> blocks;
                std::vector<block> blockdata;
                std::string line;
                bool havedata = false;

                for (std::string s; std::getline (is, s); )
                {
                    boost::smatch matches;

                    if ((s.size() > 0) && (s[0] != '#'))
                    {
                        if (havedata && boost::regex_match(s, matches, folded))
                        {
                            line += matches[1];
                            continue;
                        }

                        if (havedata)
                        {
                            process (blocks, blockdata, line);
                            havedata = false;
                        }

                        if (!havedata)
                        {
                            line = s;
                            havedata = true;
                        }
                    }
                }

                if (havedata)
                {
                    process (blocks, blockdata, line);
                }
            }
            catch (exception &e)
            {
                std::cerr << "EXCEPTION: " << e.string << "\n";
            }

            return true;
        }

    protected:
        bool process (std::vector<std::string> &blocks, std::vector<block> &blockdata, std::string &line)
        {
            static const boost::regex mime("^\\s*([\\w;=, -]+):\\s*(.*)");
            static const boost::regex attr("^([^;]*);(.*)$");
            static const boost::regex attre("^([^=]*)=(.*)$");
            boost::smatch matches;
            boost::erase_all(line, "\r");

            if (!boost::regex_match(line, matches, mime))
            {
                return false;
            }

            std::string key    = matches[1];
            std::string value  = matches[2];
            std::string keyl   = key;
            std::string valuel = value;
            std::transform(keyl.begin(),   keyl.end(),   keyl.begin(),   (int(*)(int))std::tolower);
            std::transform(valuel.begin(), valuel.end(), valuel.begin(), (int(*)(int))std::tolower);
            std::string cblocks = "";
            std::map<std::string,std::vector<std::string> > attributes;
            std::string attribute = keyl;
            bool haveattribute = false;
            if (blocks.size() > 0)
            {
                cblocks = blocks.back();
            }

            for (bool first = true; boost::regex_match(attribute, matches, attr); first = false)
            {
                if (first)
                {
                    keyl = matches[1];
                }
                else if (haveattribute)
                {
                    boost::smatch q;
                    if (boost::regex_match(std::string(matches[1]), q, attre))
                    {
                        std::string av(q[2]);
                        boost::char_separator<char> sep(",");
                        boost::tokenizer<boost::char_separator<char> > tok(av, sep);
                        std::copy(tok.begin(), tok.end(), std::back_inserter(attributes[q[1]]));
                    }
                }

                attribute = matches[2];
                haveattribute = true;
            }

            if (haveattribute)
            {
                boost::smatch q;
                if (boost::regex_match(attribute, q, attre))
                {
                    std::string av(q[2]);
                    boost::char_separator<char> sep(",");
                    boost::tokenizer<boost::char_separator<char> > tok(av, sep);
                    std::copy(tok.begin(), tok.end(), std::back_inserter(attributes[q[1]]));
                }
            }

            if (keyl == "begin")
            {
                blocks.push_back(valuel);
                blockdata.push_back(block(*this));
                blockdata.back().type       = valuel;
                blockdata.back().autocommit = true;
            }
            else if ((blocks.size() > 0) && (keyl == "end"))
            {
                blocks.pop_back();
                blockdata.pop_back();
            }
            else if (blockdata.size() > 0)
            {
                if (keyl == "uid")
                {
                    blockdata.back().uid      = valuel;
                }
                else
                {
                    blockdata.back().key       .push_back(keyl);
                    blockdata.back().value     .push_back(value);
                    blockdata.back().attributes.push_back(attributes);
                }
            }

            return true;
        }

        download &download;
    };
};

#endif
