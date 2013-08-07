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

#if !defined(FEED_DAEMON_H)
#define FEED_DAEMON_H

#include <sqlite3.h>
#include <string>
#include <feed/atom.h>
#include <feed/rss.h>
#include <feed/download.h>
#include <feed/whois.h>
#include <feed/dns.h>
#include <feed/xhtml.h>
#include <feed/html.h>

#if !defined(DEFAULT_OPTIONS)
#define DEFAULT_OPTIONS "WARDNXH"
#endif

#if !defined(DEFAULT_DATABASE)
#define DEFAULT_DATABASE "data.feed"
#endif

#if !defined(DAEMON)
#define DAEMON 1
#endif

#if DAEMON == 1
#include <stdlib.h>
#endif

namespace feed
{
    static int processDaemon (const char *opts, const char* dbfile, bool &background)
    {
        char *errmsg = 0;
        bool processAtom = false;
        bool processRSS = false;
        bool processDownload = false;
        bool processWHOIS = false;
        bool processDNS = false;
        bool processXHTML = false;
        bool processHTML = false;
        bool forkToBackground = false;

        sqlite sql(dbfile);

        for (int i = 0; opts[i]; i++)
        {
            switch (opts[i])
            {
                case 'B': forkToBackground = true; break;
                case 'A': processAtom = true; break;
                case 'R': processRSS = true; break;
                case 'D': processDownload = true; break;
                case 'W': processWHOIS = true; break;
                case 'N': processDNS = true; break;
                case 'X': processXHTML = true; break;
                case 'H': processHTML = true; break;
            }
        }

#if DAEMON == 1
        if (forkToBackground && !background && (daemon(1, 0) == 0))
        {
            background = true;
        }
#endif

        configuration configuration(sql);
        download download(configuration, processDownload);
        xml xml(configuration, download);
        atom atom(configuration, processAtom, xml);
        rss rss(configuration, processRSS, xml);
        whois whois(configuration, processWHOIS, download, xml);
        dns dns(configuration, processDNS, xml);
        xhtml xhtml(configuration, processXHTML, xml);
        html html(configuration, processHTML, xml);

        try
        {
            bool doQuit = false;

            sqlite::statement selectFeedUpdate
                ("select cmid, fid, sid, did, qid from vcommand order by priority asc",
                 sql);

            do
            {
                bool doSleep = !processDownload || download.process();

                configuration.beginTransaction();

                while (selectFeedUpdate.step() && selectFeedUpdate.row)
                {
                    enum commandtype ct;
                    int ctc;

                    selectFeedUpdate.get (0, ctc);

                    ct = (enum commandtype)ctc;

                    switch (ct)
                    {
                        case ctUpdateFeed:
                        {
                            enum servicetype st;
                            int fid;
                            int stc;

                            selectFeedUpdate.get (1, fid);
                            selectFeedUpdate.get (2, stc);
                            st = (enum servicetype)stc;

                            feed feed (configuration, fid);
                            atom.handle     (st, feed) && feed.serviceUpdate(st);
                            rss.handle      (st, feed) && feed.serviceUpdate(st);
                            whois.handle    (st, feed) && feed.serviceUpdate(st);
                            dns.handle      (st, feed) && feed.serviceUpdate(st);
                            xhtml.handle    (st, feed) && feed.serviceUpdate(st);
                            html.handle     (st, feed) && feed.serviceUpdate(st);
                            download.handle (st, feed) && feed.serviceUpdate(st);
                        }   break;
                        case ctQuery:
                        {
                            int qid;

                            std::cerr << "Q";

                            selectFeedUpdate.get (4, qid);
                            sqlite::statement stmt ("select query from query where id = ?1", sql);
                            stmt.bind (1, qid);
                            stmt.step ();
                            if (stmt.row)
                            {
                                std::string query;
                                stmt.get (0, query);
                                sqlite::statement exec (query, sql);
                                exec.step ();

                                sqlite::statement update ("update query set updated = julianday ('now') where id = ?1", sql);
                                update.bind (1, qid);
                                update.step ();
                            }
                        }
                            break;
                        case ctQuit:
                            doQuit = true;
                            sqlite::statement("delete from clientcommand where cmid = 2", sql).step();
                            break;
                        case ctDownloadComplete:
                        {
                            enum servicetype st;
                            int did;
                            int stc;
                            bool clear = false;

                            selectFeedUpdate.get (2, stc);
                            selectFeedUpdate.get (3, did);
                            st = (enum servicetype)stc;

                            clear = atom.handleCompleteDownload     (st, did) || clear;
                            clear = rss.handleCompleteDownload      (st, did) || clear;
                            clear = whois.handleCompleteDownload    (st, did) || clear;
                            clear = dns.handleCompleteDownload      (st, did) || clear;
                            clear = download.handleCompleteDownload (st, did) || clear;
                            clear = xhtml.handleCompleteDownload    (st, did) || clear;
                            clear = html.handleCompleteDownload     (st, did) || clear;

                            if (clear)
                            {
                                sqlite::statement stmt
                                    ("delete from clientcommand where sid = ?1 and did = ?2",
                                     configuration.sql);

                                stmt.bind (1, stc);
                                stmt.bind (2, did);
                                stmt.step ();
                            }
                        }
                            break;
                    }
                }
                selectFeedUpdate.reset();

                if (!download.activeTransfers())
                {
                    configuration.commitTransaction();

                    if (doSleep && !doQuit)
                    {
                        sleep(5);
                    }
                }
            }
            while (!doQuit || download.activeTransfers());

            configuration.commitTransaction();
        }
        catch (exception &e)
        {
            std::cerr << "EXCEPTION: " << e.string << "\n";
            configuration.rollbackTransaction();
            throw e;
        }
        catch (std::string &e)
        {
            std::cerr << "EXCEPTION: " << e << "\n";
            configuration.rollbackTransaction();
            throw exception(e);
        }
        catch (...)
        {
            std::cerr << "EXCEPTION: aborting\n";
            configuration.rollbackTransaction();
            throw exception("current transaction rolled back");
        }

        return 0;
    }
}

#endif
