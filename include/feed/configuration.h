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

#if !defined(FEED_CONFIGURATION_H)
#define FEED_CONFIGURATION_H

#include <feed/sql.h>
#include <feed/data-update1to2.h>

#if !defined(FEED_SCHEMAVERSION)
#define FEED_SCHEMAVERSION "2"
#endif

namespace feed
{
    class configuration
    {
    public:
        configuration(sqlite &pSQL)
            : sql(pSQL), database(pSQL),
              selectConfiguration("select value from option where otid = ?1", sql),
              begin("begin immediate transaction", sql),
              rollback("rollback", sql),
              commit("commit", sql),
              inTransaction(false)
        {
            updateConfiguration ();

            if (schemaVersion == "1")
            {
                std::cerr << "NOTICE: updating database schema from version 1 to version 2\n";

                sql.import (data::update1to2);

                updateConfiguration ();
            }

            if (schemaVersion != FEED_SCHEMAVERSION)
            {
                throw "ERROR: invalid schema version: " + schemaVersion;
            }
        }

        sqlite3 *database;
        sqlite &sql;
        std::string schemaVersion;
        std::string proxy;
        std::string userAgent;

        bool updateConfiguration (void)
        {
            readConfiguration (otSchemaVersion, schemaVersion);
            readConfiguration (otProxy, proxy);
            readConfiguration (otUserAgent, userAgent);

            return true;
        }

        bool beginTransaction (void)
        {
            return inTransaction
                || (   begin.stepReset()
                    && (inTransaction = true));
        }

        bool rollbackTransaction (void)
        {
            return inTransaction
                && rollback.stepReset()
                && !(inTransaction = false);
        }

        bool commitTransaction (void)
        {
            return inTransaction
                && commit.stepReset()
                && !(inTransaction = false);
        }

    protected:
        sqlite::statement begin;
        sqlite::statement rollback;
        sqlite::statement commit;
        sqlite::statement selectConfiguration;

        bool inTransaction;

        bool readConfiguration (enum optiontype ot, std::string &output)
        {
            int otc = (int)ot;

            selectConfiguration.bind (1, otc);

            if (selectConfiguration.step() && selectConfiguration.row)
            {
                selectConfiguration.get (0, output);
            }
            else
            {
                output = "";
            }

            selectConfiguration.reset();

            return true;
        }
    };
};

#endif
