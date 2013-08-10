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

#if !defined(FEED_SQL_H)
#define FEED_SQL_H

#include <feed/constants.h>
#include <feed/exception.h>
#include <sqlite3.h>
#include <string>
#include <iostream>

namespace feed
{
    class sqlite
    {
    public:
        sqlite(const std::string &databaseFile)
        {
            if (sqlite3_open(databaseFile.c_str(), &database) != SQLITE_OK)
            {
                throw sqlite::exception(std::string("could not open database ") + databaseFile);
            }
        }

        sqlite(const std::string &databaseFile, const char *reference)
        {
            if (sqlite3_open_v2(databaseFile.c_str(), &database, SQLITE_OPEN_READWRITE, 0) != SQLITE_OK)
            {
                std::cerr << "database '" << databaseFile << "' couuld not be opened: creating with reference data\n";

                if (sqlite3_open(databaseFile.c_str(), &database) != SQLITE_OK)
                {
                    throw sqlite::exception(std::string("could not open database ") + databaseFile);
                }

                import(reference);
            }
        }

        ~sqlite (void)
        {
            if (sqlite3_close(database) != SQLITE_OK)
            {
                throw sqlite::exception("could not close database", *this);
            }
        }

        class exception : public feed::exception
        {
        public:
            exception(const std::string &pString)
                : feed::exception(pString) {}

            exception(const std::string &pString, const sqlite &pSQL)
                : feed::exception(pString + ": " + sqlite3_errmsg(pSQL.database)) {}
        };

        class statement
        {
        public:
            statement(const std::string &pStatement, sqlite &pSQL)
                : sql(pSQL), row(false)
            {
                if (sqlite3_prepare_v2
                        (sql.database,
                         pStatement.c_str(),
                         -1, &stmt, 0) != SQLITE_OK)
                {
                    throw sqlite::exception("sqlite3_prepare_v2", sql);
                }
            }

            ~statement(void)
            {
                if (sqlite3_finalize (stmt) != SQLITE_OK)
                {
                    throw sqlite::exception("sqlite3_finalize", sql);
                }
            }

            bool step (void)
            {
                switch (sqlite3_step (stmt))
                {
                    case SQLITE_ROW:
                        row = true;
                        return true;
                    case SQLITE_OK:
                    case SQLITE_DONE:
                        row = false;
                        return true;
                    default:
                        throw sqlite::exception("sqlite3_step", sql);
                }

                return false;
            }

            bool reset (void)
            {
                if (sqlite3_reset (stmt) != SQLITE_OK)
                {
                    throw sqlite::exception("sqlite3_reset", sql);
                    return false;
                }

                row = false;

                return true;
            }

            bool stepReset (void)
            {
                return step() && reset();
            }

            template <typename T>
            bool bind (int i, const T &value)
            {
                if (sqlite3_bind_null (stmt, i) != SQLITE_OK)
                {
                    throw sqlite::exception("sqlite3_bind_int64", sql);
                    return false;
                }

                return true;
            }

            template <typename T>
            bool get (int i, T &value)
            {
                if (!row)
                {
                    return false;
                }

                return sqlite3_column_type (stmt, i) == SQLITE_NULL;
            }

            int getColumnCount (void) const
            {
                return sqlite3_column_count(stmt);
            }

            operator sqlite3_stmt * (void)
            {
                return stmt;
            }

            bool row;

        protected:
            sqlite &sql;
            sqlite3_stmt *stmt;
        };

        statement prepare (const std::string &pStatement)
        {
            return statement (pStatement, *this);
        }

        bool execute (const std::string &pStatement)
        {
            statement statement(pStatement, *this);
            return statement.stepReset();
        }

        operator sqlite3 * (void)
        {
            return database;
        }

        bool import (const char *data)
        {
            size_t pos = 0;
            const char *tail = data;

            do
            {
                const char *ntail;
                sqlite3_stmt *stmt = 0;
                if (sqlite3_prepare_v2 (database, tail, -1, &stmt, &ntail) == SQLITE_OK)
                {
                    if (sqlite3_step(stmt) == SQLITE_ERROR)
                    {
                        std::cerr << "import: " << sqlite3_errmsg(database) << "\n";
                    }
                    sqlite3_finalize(stmt);
                    tail = ntail;
                }
            } while ((tail != 0) && (*tail != (char)0));

            return true;
        }

    protected:
        sqlite3 *database;
    };

    template <>
    bool sqlite::statement::bind (int i, const long long &value)
    {
        sqlite3_int64 v = value;

        if (sqlite3_bind_int64 (stmt, i, v) != SQLITE_OK)
        {
            throw sqlite::exception("sqlite3_bind_int64", sql);
            return false;
        }

        return true;
    }

    template <>
    bool sqlite::statement::bind (int i, const int &value)
    {
        if (sqlite3_bind_int (stmt, i, value) != SQLITE_OK)
        {
            throw sqlite::exception("sqlite3_bind_int", sql);
            return false;
        }

        return true;
    }

    template <>
    bool sqlite::statement::bind (int i, const std::string &value)
    {
        if (sqlite3_bind_text (stmt, i, value.data(), value.size(), SQLITE_TRANSIENT) != SQLITE_OK)
        {
            throw sqlite::exception("sqlite3_bind_text", sql);
            return false;
        }

        return true;
    }

    template <>
    bool sqlite::statement::bind (int i, const double &value)
    {
        if (sqlite3_bind_double (stmt, i, value) != SQLITE_OK)
        {
            throw sqlite::exception("sqlite3_bind_double", sql);
            return false;
        }

        return true;
    }

    template <>
    bool sqlite::statement::get (int i, long long &value)
    {
        if (!row)
        {
            return false;
        }

        sqlite3_int64 v = sqlite3_column_int64 (stmt, i);

        value = (long long)v;

        return true;
    }

    template <>
    bool sqlite::statement::get (int i, int &value)
    {
        if (!row)
        {
            return false;
        }

        value = sqlite3_column_int (stmt, i);

        return true;
    }

    template <>
    bool sqlite::statement::get (int i, std::string &value)
    {
        if (!row)
        {
            return false;
        }

        const char *v = (const char*)sqlite3_column_text (stmt, i);

        value = v ? v : "";

        return v != 0;
    }

    template <>
    bool sqlite::statement::get (int i, double &value)
    {
        if (!row)
        {
            return false;
        }

        value = sqlite3_column_double (stmt, i);

        return true;
    }
};

#endif
