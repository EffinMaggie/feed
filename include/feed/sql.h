/**\file
 *
 * \copyright
 * Copyright (c) 2013-2014, FEED Project Members
 * \copyright
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * \copyright
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * \copyright
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * \see Project Documentation: http://ef.gy/documentation/feed
 * \see Project Source Code: http://git.becquerel.org/jyujin/feed.git
 */

#if !defined(FEED_SQL_H)
#define FEED_SQL_H

#include <feed/constants.h>
#include <feed/exception.h>
#include <ef.gy/sqlite.h>

namespace feed
{
    /**\brief SQLite3 class
     *
     * Imports the SQLite3 class from libefgy. This used to be in FEED itself,
     * but the class should come in handy in other places too, so it has been
     * merged into libefgy.
     */
    typedef efgy::database::sqlite sqlite;
};

#endif
