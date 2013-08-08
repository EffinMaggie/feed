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

#if !defined(FEED_MACRO_H)
#define FEED_MACRO_H

#include <feed/constants.h>
#include <feed/configuration.h>
#include <feed/query.h>
#include <vector>
#include <fstream>

namespace feed
{
    class macro
    {
    public:
        macro(configuration &pConfiguration, int mID)
            : configuration(pConfiguration), id(mID), macroName()
        {
            sqlite::statement read
                ("select id, macro, flag from querymacro where id = ?1 order by line asc",
                 configuration.sql);

            read.bind (1, id);
            read.step ();
            if (!read.row)
            {
                throw exception("tried to initialise a feed::macro object with a nonexistent id");
            }

            read.get (0, id);
            read.get (1, macroName);

            while (read.row)
            {
                std::string s;
                read.get (2, s);
                queries.push_back (s);
                read.step();
            }
        }

        macro(configuration &pConfiguration, const std::string &pMacro)
            : configuration(pConfiguration), id(0), macroName(pMacro)
        {
            sqlite::statement read
                ("select id, macro, flag from querymacro where macro = ?1 order by line asc",
                 configuration.sql);

            read.bind (1, macroName);
            read.step ();
            if (!read.row)
            {
                throw exception("tried to initialise a feed::macro object with a nonexistent reference: " + macroName);
            }

            read.get (0, id);
            read.get (1, macroName);

            while (read.row)
            {
                std::string s;
                read.get (2, s);
                queries.push_back (s);
                read.step();
            }
        }

        int id;
        std::string macroName;
        std::vector<std::string> queries;

        bool run
            (std::vector<std::string>::iterator &arg,
             const std::vector<std::string>::const_iterator &end)
        {
            for (std::vector<std::string>::iterator it = queries.begin();
                 it != queries.end();
                 it++)
            {
                query q(configuration, *it);
                if (!q.run (arg, end))
                {
                    return false;
                }
            }

            return true;
        }

    protected:
        configuration &configuration;
    };
};

#endif
