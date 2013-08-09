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

#define DEFAULT_OPTIONS "BWARDNXH"
#define DAEMON 1

#include <cstdlib>
#include <feed/daemon.h>
#include <feed/query.h>
#include <feed/macro.h>

int main (int argc, char**argv)
{
    const char *opts = DEFAULT_OPTIONS;
    const char *dbfile = DEFAULT_DATABASE;
    bool skipDaemon = false;
    bool initialiseDatabase = true;
    bool doClient = true;

    std::vector<std::string> cmd;

    for (int i = 0; (i < argc) && (argv[i]); i++)
    {
        std::string s = argv[i];

        if (s == "--daemon")
        {
            if (i++, (i < argc) && (argv[i]))
            {
                opts = argv[i];
            }
        }
        else if (s == "--database")
        {
            if (i++, (i < argc) && (argv[i]))
            {
                dbfile = argv[i];
            }
        }
        else if (s == "--skip-daemon")
        {
            skipDaemon = true;
        }
        else if (i > 0)
        {
            cmd.push_back (s);
        }
    }

    if (opts == DEFAULT_OPTIONS)
    {
        const char *envopts = std::getenv ("FEEDD_OPTIONS");
        dbfile = envopts ? envopts : dbfile;
    }

    if (dbfile == DEFAULT_DATABASE)
    {
        const char *envdb = std::getenv ("FEED_DATABASE");
        dbfile = envdb ? envdb : dbfile;
    }

    if (initialiseDatabase)
    {
        feed::sqlite (dbfile, feed::data::feed);
    }

    dbfile = realpath (dbfile, 0);

    if (dbfile == 0)
    {
        std::cerr << "ABORTED: call to realpath(dbfile) failed\n";
        return -3;
    }

    if (!skipDaemon)
    {
        bool background = false;

        try
        {
            doClient = (feed::processDaemon (std::string(opts) + "B", dbfile, background) == 1);
        }
        catch (feed::exception &e)
        {
            std::cerr << "ABORTED: " << e.string << "\n";
            return -1;
        }
    }

    if (doClient)
    {
        feed::sqlite sql
            (dbfile, feed::data::feed);
        feed::configuration configuration
            (sql);

        if (cmd.size() == 0)
        {
            cmd.push_back ("new");
        }

        for (std::vector<std::string>::iterator it = cmd.begin();
             it != cmd.end();
             it++)
        {
            const std::string s(*it);
            std::string ex;

            try
            {
                feed::query query (configuration, s);
                query.run(it, cmd.end());
                continue;
            }
            catch (feed::exception &e)
            {
                ex = e.string;
            }

            try
            {
                feed::macro macro (configuration, s);
                macro.run(it, cmd.end());
                continue;
            }
            catch (feed::exception &e)
            {
                std::cerr << "INVALID QUERY: " << s << ": " << ex << "\n";
                std::cerr << "INVALID MACRO: " << s << ": " << e.string << "\n";
            }
        }
    }

    return 0;
}
