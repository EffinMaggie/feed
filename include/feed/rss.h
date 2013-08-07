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

#if !defined(FEED_RSS_H)
#define FEED_RSS_H

#include <feed/handler.h>
#include <feed/entry.h>
#include <feed/xml.h>

namespace feed
{
    class rss : public handler
    {
    public:
        rss(configuration &pConfiguration, const bool &pEnabled, xml &pXml)
            : handler(pConfiguration, stRSS, pEnabled), xml(pXml)
        {}

        virtual bool handle
            (const enum servicetype &st,
             feed::feed &feed)
        {
            if (!enabled)
            {
                return false;
            }

            if (st != stRSS)
            {
                return false;
            }
            std::cerr << "R";

            try
            {
                xml::parser parser = xml.parse (feed.source);

                if (parser.updateContext ("/rss/channel/item[1]"))
                do
                {
                    std::string xid = parser.evaluate ("atom:id");
                    if (xid == "")
                    {
                        xid = parser.evaluate ("guid");
                    }
                    if (xid == "")
                    {
                        xid = parser.evaluate ("link");
                    }
                    if (xid == "")
                    {
                        continue;
                    }

                    entry entry(context, xid);

                    entry.addMeta (mtSourceFeed, feed.source);

                    std::string value;
                    std::string contentType;

                    if ((value = parser.evaluate("title")) != "")
                    {
                        entry.addMeta (mtTitle, value);
                    }
                    if ((value = parser.evaluate("atom:summary")) != "")
                    {
                        entry.addMeta (mtAbstract, value);
                    }
                    else if ((value = parser.evaluate("description")) != "")
                    {
                        entry.addMeta (mtAbstract, value);
                    }
                    if ((value = parser.evaluate("atom:updated")) != "")
                    {
                        entry.addMeta (mtUpdated, value);
                    }
                    else if ((value = parser.evaluate("pubDate")) != "")
                    {
                        entry.addMeta (mtUpdated, value);
                    }
                    if ((value = parser.evaluate("atom:published")) != "")
                    {
                        entry.addMeta (mtPublished, value);
                    }
                    else if ((value = parser.evaluate("pubDate")) != "")
                    {
                        entry.addMeta (mtPublished, value);
                    }
                    if ((value = parser.evaluate("atom:content/@type")) != "")
                    {
                        entry.addMeta (mtContentMIME, value);
                        contentType = value;
                    }
                    if ((value = parser.evaluate("atom:content/@src")) != "")
                    {
                        entry.addMeta (mtCanonicalURI, value);
                    }
                    else if ((value = parser.evaluate("atom:link[@rel='sel'][1]/@href")) != "")
                    {
                        entry.addMeta (mtCanonicalURI, value);
                    }
                    else if ((value = parser.evaluate("atom:link[@rel='alternate'][1]/@href")) != "")
                    {
                        entry.addMeta (mtCanonicalURI, value);
                    }
                    else if ((value = parser.evaluate("atom:link[1]/@href")) != "")
                    {
                        entry.addMeta (mtCanonicalURI, value);
                    }
                    else if ((value = parser.evaluate("atom:id")) != "")
                    {
                        entry.addMeta (mtCanonicalURI, value);
                    }
                    else if ((value = parser.evaluate("link")) != "")
                    {
                        entry.addMeta (mtCanonicalURI, value);
                    }
                    else if ((value = parser.evaluate("enclosure/@url")) != "")
                    {
                        entry.addMeta (mtCanonicalURI, value);
                    }
                    if ((value = parser.evaluateToFragment("atom:content/node() | atom:content/text()")) != "")
                    {
                        entry.addMeta (mtContent, value);
                    }

                    std::cerr << "r";
                }
                while (parser.updateContext ("following-sibling::item[1]"));
            }
            catch (exception &e)
            {
                // std::cerr << "EXCEPTION: " << e.string << "\n";
                return true;
            }

            return true;
        }

    protected:
        xml &xml;
    };
};

#endif
