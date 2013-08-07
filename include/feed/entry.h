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

#if !defined(FEED_ENTRY_H)
#define FEED_ENTRY_H

#include <feed/constants.h>
#include <feed/configuration.h>

namespace feed
{
    class entry
    {
    public:
        entry(configuration &pConfiguration, int pID)
            : configuration(pConfiguration), id(pID), xid()
        {
            sqlite::statement stmt ("select xid from entry where xid = ?1", configuration.sql);
            stmt.bind (1, id);
            stmt.step ();
            if (stmt.row)
            {
                stmt.get (0, xid);
            }
            else
            {
                throw exception("tried to initialise a feed::entry object with a nonexistent id");
            }
        }

        entry(configuration &pConfiguration, const std::string &pXID)
            : configuration(pConfiguration), xid(pXID), id(-1)
        {
            bool addNew = false;
            {
                sqlite::statement stmt ("select id from entry where xid = ?1", configuration.sql);
                stmt.bind (1, xid);
                stmt.step ();
                if (stmt.row)
                {
                    stmt.get (0, id);
                }
                else
                {
                    addNew = true;
                }
            }

            if (addNew)
            {
                sqlite::statement stmt ("insert or replace into entry (xid) values (?1)", configuration.sql);
                stmt.bind (1, xid);
                stmt.step ();

                id = sqlite3_last_insert_rowid(configuration.sql);
            }
        }

        int id;
        std::string xid;

        bool addMeta (enum metatype mt, const std::string &value)
        {
            int mtc = (int)mt;
            sqlite::statement stmt ("insert or replace into entrymeta (eid, mid, value) values (?1, ?2, ?3)", configuration.sql);
            stmt.bind (1, id);
            stmt.bind (2, mtc);
            stmt.bind (3, value);
            stmt.step ();

            return true;
        }

    protected:
        configuration &configuration;
    };
};

#endif
