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

#if !defined(FEED_HANDLER_H)
#define FEED_HANDLER_H

#include <feed/exception.h>
#include <feed/feed.h>
#include <iostream>

namespace feed
{
    class handler
    {
    public:
        handler(configuration &pConfiguration, const enum servicetype &pServiceType, const bool &pEnabled = false)
            : context(pConfiguration), serviceType(pServiceType), enabled(pEnabled)
        {}

        virtual bool handle
            (const enum servicetype &st,
             feed::feed &feed) = 0;

        virtual bool handleCompleteDownload
            (const enum servicetype &st,
             int downloadID)
        {
            if (!enabled)
            {
                return false;
            }

            if (st != serviceType)
            {
                return false;
            }

            return true;
        }

    protected:
        const enum servicetype serviceType;
        bool enabled;
        configuration &context;
    };
};

#endif
