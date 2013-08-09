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

#if !defined(FEED_FEED_H)
#define FEED_FEED_H

#include <feed/constants.h>
#include <feed/configuration.h>

namespace feed
{
    class feed
    {
    public:
        feed(configuration &pConfiguration, int pID)
            : configuration(pConfiguration), id(pID), source()
        {
            sqlite::statement read
                ("select source from feed where id = ?1",
                 configuration.sql);

            read.bind (1, id);
            read.step ();
            if (!read.row)
            {
                throw exception("tried to initialise a feed::feed object with a nonexistent id");
            }

            read.get (0, source);
        }

        int id;
        std::string source;

        bool serviceUpdate (const enum servicetype &st)
        {
            const int stc = (const int)st;
            sqlite::statement update
                ("update feedservice set updated = julianday('now') where fid = ?1 and sid = ?2",
                 configuration.sql);

            update.bind (1, id);
            update.bind (2, stc);
            update.step ();

            return true;
        }

    protected:
        configuration &configuration;
    };
};

#endif
