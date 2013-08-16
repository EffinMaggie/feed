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
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <feed/atom.h>
#include <feed/rss.h>
#include <feed/download.h>
#include <feed/whois.h>
#include <feed/dns.h>
#include <feed/xhtml.h>
#include <feed/html.h>
#include <feed/ical.h>
#include <feed/data-feed.h>

#if !defined(DEFAULT_OPTIONS)
#define DEFAULT_OPTIONS "WARDNXHI"
#endif

#if !defined(DEFAULT_DATABASE)
#define DEFAULT_DATABASE "data.feed"
#endif

#if !defined(DAEMON)
#define DAEMON 1
#endif

#if DAEMON == 1
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#endif

namespace feed
{
    static int processDaemon (const std::string &opts, const std::string &dbfile, bool &background)
    {
        bool processAtom = false;
        bool processRSS = false;
        bool processDownload = false;
        bool processWHOIS = false;
        bool processDNS = false;
        bool processXHTML = false;
        bool processHTML = false;
        bool processICal = false;
        bool allowSecondary = false;
        bool forkToBackground = false;
        bool recordInstance = true;

        for (int i = 0; i < opts.size(); i++)
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
                case 'I': processICal = true; break;
                case 'S': allowSecondary = true; break;
            }
        }

        if (!allowSecondary)
        {
            bool running = false;

            sqlite sql(dbfile, data::feed);

            sqlite::statement stmt ("select pid from instance", sql);
            while (stmt.step () && stmt.row)
            {
                int pid;
                if (stmt.get (0, pid))
                {
                    if (kill (pid, 0))
                    {
                        switch (errno)
                        {
                            /* pid is still active, but as different user */
                            case EPERM:
                                running = true;
                                break;
                            /* pid is no longer active */
                            case ESRCH:
                            {
                                stmt.reset ();
                                sqlite::statement removePID
                                    ("delete from instance where pid = ?1", sql);
                                removePID.bind (1, pid);
                                removePID.step ();
                            }
                            default: break;
                        }
                    }
                    else
                    {
                        running = true;
                    }
                }
            }

            if (running)
            {
                return 1;
            }
        }

#if DAEMON == 1
        if (forkToBackground && !background)
        {
            switch (fork())
            {
                case 0:
                    /* in child process */
                {
                    int fd;
#if defined(TIOCNOTTY)
                    fd = open("/dev/tty", O_RDWR);
                    if (fd >= 0) {
                        ioctl(fd, TIOCNOTTY, 0);
                        close(fd);
                    }
#endif

                    fd = open ("/dev/null", O_RDWR);
                    if (fd >= 0)
                    {
                        dup2 (fd, 0);
                        dup2 (fd, 1);
                        dup2 (fd, 2);
                    }

                    chdir ("/");
                    setsid ();
#if defined(SIGTTOU)
                    signal (SIGTTOU, SIG_IGN);
#endif
#if defined(SIGTTIN)
                    signal (SIGTTIN, SIG_IGN);
#endif
#if defined(SIGTSTP)
                    signal (SIGTSTP, SIG_IGN);
#endif
#if defined(SIGHUP)
                    signal (SIGHUP, SIG_IGN);
#endif
                }
                    background = true;

                    break;
                case -1:
                    /* no child process created */
                    throw exception ("could not spawn child process");
                    return -1;
                default:
                    background = true;

                    return 1;
            }
        }
#endif

        sqlite sql(dbfile, data::feed);

        if (recordInstance)
        {
            sqlite::statement stmt ("insert or ignore into instance (pid) values (?1)", sql);
            stmt.bind (1, getpid());
            stmt.step ();
        }

        configuration configuration(sql);
        download download(configuration, processDownload);
        xml xml(configuration, download);
        atom atom(configuration, processAtom, xml);
        rss rss(configuration, processRSS, xml);
        whois whois(configuration, processWHOIS, download, xml);
        dns dns(configuration, processDNS, xml);
        xhtml xhtml(configuration, processXHTML, xml);
        html html(configuration, processHTML, xml);
        ical ical(configuration, processICal, download);

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
                            ical.handle     (st, feed) && feed.serviceUpdate(st);
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
                            clear = ical.handleCompleteDownload     (st, did) || clear;

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

        sqlite::statement removePID
            ("delete from instance where pid = ?1", sql);
        removePID.bind (1, getpid());
        removePID.step ();

        return 0;
    }
}

#endif
