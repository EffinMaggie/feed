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

#if !defined(FEED_WHOIS_H)
#define FEED_WHOIS_H

#include <feed/handler.h>
#include <feed/entry.h>
#include <feed/xml.h>
#include <feed/download.h>
#include <boost/regex.hpp>
#include <sstream>

namespace feed
{
    class whois : public handler
    {
    public:
        whois(configuration &pConfiguration, const bool &pEnabled, download &pDownload, xml &pXml)
            : handler(pConfiguration, stWhois, pEnabled), download(pDownload), xml(pXml),
              insertFeed("insert or ignore into feed (sid, source) values (2, ?1)", context.sql),
              insertWhoisDownload("insert or ignore into whoisdownload (fid, uri, payload, query) values (?1, ?2, ?3, ?4)", context.sql),
              insertWhois("insert or ignore into whois (query) values (?1)", context.sql),
              insertWhoisDetail("insert or replace into whoisdetail (wid, line, key, value) values (?1, ?2, ?3, ?4)", context.sql),
              selectWhoisID("select id from whois where query = ?1", context.sql),
              selectWhoisQuery("select query from whoisdownload, download where whoisdownload.uri = download.uri and whoisdownload.payload = download.payload and download.id = ?1", context.sql)
        {}

        virtual bool handle
            (const enum servicetype &st,
             feed::feed &feed)
        {
            if (!enabled)
            {
                return false;
            }

            if (st != stWhois)
            {
                return false;
            }
            std::cerr << "W";

            std::string scheme;
            std::string opaque;
            std::string authority;
            std::string server;
            std::string user;
            int port;
            std::string path;
            std::string query;
            std::string fragment;

            if (xml.parseURI
                    (feed.source,
                     scheme, opaque, authority, server,
                     user, port, path, query, fragment) == false)
            {
                std::cerr << "could not parse URI: " << feed.source << "\n";
                return true;
            }

            if (scheme != "whois")
            {
                static const boost::regex domain("([^\\.]+\\.)*([^\\.]+\\.([^\\.]+))$");
                boost::smatch matches;
                std::string tld;
                std::string dmn;

                if (boost::regex_match(server, matches, domain))
                {
                    tld = matches[3];
                    dmn = matches[2];

                    insertFeed.bind (1, "whois://" + tld + ".whois-servers.net/#" + dmn);
                    insertFeed.stepReset();
                }
                else
                {
                    insertFeed.bind (1, "whois://whois.iana.org/#" + server);
                    insertFeed.stepReset();
                }
            }
            else
            {
                const std::string uri ("telnet://" + server + ":43");
                const std::string payload (fragment + "\r\n");
                download.queueTransfer (uri, payload, stWhois);

                insertWhoisDownload.bind (1, feed.id);
                insertWhoisDownload.bind (2, uri);
                insertWhoisDownload.bind (3, payload);
                insertWhoisDownload.bind (4, fragment);
                insertWhoisDownload.stepReset ();
            }

            return true;
        }

        virtual bool handleCompleteDownload
            (const enum servicetype &st,
             int downloadID)
        {
            if (!enabled)
            {
                return false;
            }

            if (st != serviceType)
            {
                return false;
            }

            download::data data = download.retrieve (downloadID);
            std::string qid;

            selectWhoisQuery.bind (1, downloadID);
            selectWhoisQuery.step ();
            selectWhoisQuery.get  (0, qid);
            selectWhoisQuery.reset ();

            query query(*this, qid);

            static const boost::regex mime("\\s*([\\w -]+):\\s*(.*)\\s*");

            std::istringstream is(data.content);

            int line = 0;

            for (std::string s; std::getline (is, s); )
            {
                boost::smatch matches;

                if ((s.size() > 0) && (s[0] != '#'))
                {
                    if (boost::regex_match(s, matches, mime))
                    {
                        std::string key = matches[1];
                        std::string value = matches[2];
                        query.addDetail (line++, key, value);

                        if (key == "ReferralServer")
                        {
                            insertFeed.bind (1, value + "/#" + qid);
                            insertFeed.stepReset();
                        }
                    }
                }
            }

            return true;
        }

    protected:
        sqlite::statement insertFeed;
        sqlite::statement insertWhoisDownload;
        sqlite::statement insertWhois;
        sqlite::statement insertWhoisDetail;
        sqlite::statement selectWhoisID;
        sqlite::statement selectWhoisQuery;

        download &download;
        xml &xml;

        class query
        {
        public:
            query(whois &pWhois, const std::string &pQID)
                : whois(pWhois), qid(pQID), id(-1)
            {
                bool addNew = false;
                {
                    whois.selectWhoisID.bind (1, qid);
                    whois.selectWhoisID.step ();
                    if (whois.selectWhoisID.row)
                    {
                        whois.selectWhoisID.get (0, id);
                    }
                    else
                    {
                        addNew = true;
                    }
                    whois.selectWhoisID.reset ();
                }

                if (addNew)
                {
                    whois.insertWhois.bind (1, qid);
                    whois.insertWhois.step ();

                    id = sqlite3_last_insert_rowid(whois.context.sql);

                    whois.insertWhois.reset();
                }
            }

            int id;
            std::string qid;

            bool addDetail (int line, const std::string &key, const std::string &value)
            {
                whois.insertWhoisDetail.bind (1, id);
                whois.insertWhoisDetail.bind (2, line);
                whois.insertWhoisDetail.bind (3, key);
                whois.insertWhoisDetail.bind (4, value);
                whois.insertWhoisDetail.stepReset();

                return true;
            }

        protected:
            whois &whois;
        };
    };
};

#endif
