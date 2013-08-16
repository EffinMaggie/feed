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

#if !defined(FEED_ICAL_H)
#define FEED_ICAL_H

#include <feed/handler.h>
#include <feed/entry.h>
#include <feed/xml.h>
#include <feed/download.h>
#include <boost/regex.hpp>
#include <sstream>

namespace feed
{
    class ical : public handler
    {
    public:
        ical(configuration &pConfiguration, const bool &pEnabled, download &pDownload)
            : handler(pConfiguration, stICal, pEnabled), download(pDownload)
        {}

        virtual bool handle
            (const enum servicetype &st,
             feed::feed &feed)
        {
            if (!enabled)
            {
                return false;
            }

            if (st != stICal)
            {
                return false;
            }
            std::cerr << "I";

            try
            {
                download::data data = download.retrieve (feed.source);
            }
            catch (exception &e)
            {
                std::cerr << "EXCEPTION: " << e.string << "\n";
            }

            return true;
        }

    protected:
        download &download;
    };
};

#endif
