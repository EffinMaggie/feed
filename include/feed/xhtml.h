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

#if !defined(FEED_XHTML_H)
#define FEED_XHTML_H

#include <feed/handler.h>
#include <feed/entry.h>
#include <feed/xml.h>

namespace feed
{
    class xhtml : public handler
    {
    public:
        xhtml(configuration &pConfiguration, const bool &pEnabled, xml &pXml)
            : handler(pConfiguration, stXHTML, pEnabled), xml(pXml),
              insertFeed("insert or ignore into feed (source) values (?1)", context.sql)
        {}

        virtual bool handle
            (const enum servicetype &st,
             feed::feed &feed)
        {
            if (!enabled)
            {
                return false;
            }

            if (st != stXHTML)
            {
                return false;
            }
            std::cerr << "X";

            try
            {
                xml::parser parser = xml.parse (feed.source);

                if (parser.updateContext ("/xhtml:html/xhtml:head/xhtml:link[@rel='alternate'][(@type='application/atom+xml') or (@type='application/rss+xml')][1]"))
                do
                {
                    std::string href = parser.evaluate ("@href");
                    if (href == "")
                    {
                        continue;
                    }

                    insertFeed.bind (1, xml.buildURI(href, feed.source));
                    insertFeed.stepReset();

                    std::cerr << "x";
                }
                while (parser.updateContext ("following-sibling::xhtml:link[@rel='alternate'][(@type='application/atom+xml') or (@type='application/rss+xml')][1]"));
            }
            catch (exception &e)
            {
                // std::cerr << "EXCEPTION: " << e.string << "\n";
                return true;
            }

            return true;
        }

    protected:
        sqlite::statement insertFeed;

        xml &xml;
    };
};

#endif
