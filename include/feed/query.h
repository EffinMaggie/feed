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

#if !defined(FEED_QUERY_H)
#define FEED_QUERY_H

#include <feed/constants.h>
#include <feed/configuration.h>
#include <vector>
#include <fstream>

namespace feed
{
    class query
    {
    public:
        query(configuration &pConfiguration, int qID)
            : configuration(pConfiguration), id(qID), flag(), queryString()
        {
            sqlite::statement read
                ("select id, flag, query, shell, parameters from query where id = ?1",
                 configuration.sql);

            read.bind (1, id);
            read.step ();
            if (!read.row)
            {
                throw exception("tried to initialise a feed::query object with a nonexistent id");
            }

            read.get (0, id);
            read.get (1, flag);
            read.get (2, queryString);
            read.get (3, shell);
            read.get (4, parameters);
        }

        query(configuration &pConfiguration, const std::string &pFlag)
            : configuration(pConfiguration), id(0), flag(pFlag), queryString()
        {
            sqlite::statement read
                ("select id, flag, query, shell, parameters from query where flag = ?1",
                 configuration.sql);

            read.bind (1, flag);
            read.step ();
            if (!read.row)
            {
                throw exception("tried to initialise a feed::query object with a nonexistent reference: " + flag);
            }

            read.get (0, id);
            read.get (1, flag);
            read.get (2, queryString);
            read.get (3, shell);
            read.get (4, parameters);
        }

        int id;
        std::string flag;
        std::string queryString;
        int shell;
        int parameters;

        bool run
            (std::vector<std::string>::iterator &arg,
             const std::vector<std::string>::const_iterator &end)
        {
            std::cout << flag << ":\n";

            sqlite::statement q(queryString, configuration.sql);
            const int columns = q.getColumnCount();

            for (int i = 0; i < parameters; i++)
            {
                arg++;
                if (arg != end)
                {
                    const std::string s(*arg);
                    q.bind (1+i, s);
                }
                else
                {
                    arg--;
                    throw exception ("not enough arguments for query");
                }
            }

            while (q.step() && q.row)
            {
                if (shell)
                {
                    std::string command;
                    std::string data;

                    q.get (0, command);
                    q.get (1, data);

                    q.reset();

                    std::string tempfile = tmpnam(0);
                    {
                        std::ofstream o;
                        o.open(tempfile.c_str());
                        o << data;
                        o.close();
                        std::string shellCommand ("export filename=" + tempfile + " && " + command);
                        system (shellCommand.c_str());
                    }
                    unlink (tempfile.c_str());

                    break;
                }
                else
                {
                    std::string s;
                    for (int i = 0; i < columns; i++)
                    {
                        if (q.get (i, s))
                        {
                            std::cout << s << "\t";
                        }
                    }
                }
                std::cout << "\n";
            }

            return true;
        }

    protected:
        configuration &configuration;
    };
};

#endif
