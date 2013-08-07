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

#if !defined(FEED_EXCEPTION_H)
#define FEED_EXCEPTION_H

namespace feed
{
    class exception
    {
    public:
        exception(const std::string &pString)
            : string(pString)
        {}

        const std::string string;
    };

    class exceptionCURL : public exception
    {
    public:
        exceptionCURL(void)
            : exception("CURL")
        {}

        exceptionCURL(const std::string &pString)
            : exception(pString)
        {}
    };
}

#include <feed/configuration.h>

namespace feed
{
    class exceptionSQLITE : public exception
    {
    public:
        exceptionSQLITE(void)
            : exception("SQLITE")
        {}

        exceptionSQLITE(const std::string &pString)
            : exception(pString)
        {}

        exceptionSQLITE(const std::string &pString, configuration &pConfiguration)
            : exception(pString + ": " + sqlite3_errmsg(pConfiguration.database))
        {}
    };
};

#endif
