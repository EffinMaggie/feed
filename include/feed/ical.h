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
    public:
        ical(configuration &pConfiguration, const bool &pEnabled, download &pDownload)
            : handler(pConfiguration, stICal, pEnabled), download(pDownload)
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

                std::vector<std::string> block;
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
                            process (block, line);
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
                    process (block, line);
                }
            }
            catch (exception &e)
            {
                std::cerr << "EXCEPTION: " << e.string << "\n";
            }

            return true;
        }

        bool process (std::vector<std::string> &block, std::string &line)
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
            std::string cblock = "";
            std::map<std::string,std::vector<std::string> > attributes;
            std::string attribute = keyl;
            bool haveattribute = false;
            if (block.size() > 0)
            {
                cblock = block.back();
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
                block.push_back(valuel);
            }
            else if ((block.size() > 0) && (keyl == "end"))
            {
                block.pop_back();
            }
            else
            {
                std::cerr << cblock << "::" << keyl << "(" << attributes.size() << ")" << "=" << value << "\n";
                if (attributes.size() > 0)
                {
                    for (std::map<std::string,std::vector<std::string> >::iterator it = attributes.begin();
                         it != attributes.end();
                         it++)
                    {
                        std::cerr << "[" << it->first << "] = ";
                        for (std::vector<std::string>::iterator it2 = it->second.begin();
                             it2 != it->second.end();
                             it2++)
                        {
                            std::cerr << *it2 << " ";
                        }
                        std::cerr << "\n";
                    }
                }
            }

            return true;
        }

    protected:
        download &download;
    };
};

#endif
