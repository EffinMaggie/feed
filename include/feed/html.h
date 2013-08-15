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

#if !defined(FEED_HTML_H)
#define FEED_HTML_H

#include <feed/handler.h>
#include <feed/entry.h>
#include <feed/xml.h>

namespace feed
{
    class html : public handler
    {
    public:
        html(configuration &pConfiguration, const bool &pEnabled, xml &pXml)
            : handler(pConfiguration, stHTML, pEnabled), xml(pXml),
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

            if (st != stHTML)
            {
                return false;
            }
            std::cerr << "H";

            try
            {
                xml::parser parser = xml.parse (feed.source, true);

				entry entry(context, feed.source);

				std::string value;

                entry.addMeta (mtCanonicalURI, feed.source);

                if ((value = parser.evaluate("/xhtml:html/xhtml:head/xhtml:title")) != "")
                {
                    entry.addMeta (mtTitle, value);
                }
                else if ((value = parser.evaluate("/html/head/title")) != "")
                {
                    entry.addMeta (mtTitle, value);
                }
                if ((value = parser.evaluate("/xhtml:html/xhtml:head/xhtml:meta[@name='author']/@content")) != "")
                {
                    entry.addMeta (mtAuthorName, value);
                }
                else if ((value = parser.evaluate("/html/head/meta[@name='author']/@content")) != "")
                {
                    entry.addMeta (mtAuthorName, value);
                }
                if ((value = parser.evaluate("/xhtml:html/xhtml:head/xhtml:meta[@name='description']/@content")) != "")
                {
                    entry.addMeta (mtAbstract, value);
                }
                else if ((value = parser.evaluate("/html/head/meta[@name='description']/@content")) != "")
                {
                    entry.addMeta (mtAbstract, value);
                }

                if (parser.updateContext ("/html/head/link[@rel='alternate'][(@type='application/atom+xml') or (@type='application/rss+xml')][1]"))
                do
                {
                    std::string href = parser.evaluate ("@href");
                    if (href == "")
                    {
                        continue;
                    }

                    href = xml.buildURI(href, feed.source);

                    insertFeed.bind (1, href);
                    insertFeed.stepReset();

				    typeof(entry) n (context, href);
                    n.addMeta (mtCanonicalURI, href);
                    entry.linkTo (rLinksTo, n);

                    std::cerr << "h";
                }
                while (parser.updateContext ("following-sibling::link[@rel='alternate'][(@type='application/atom+xml') or (@type='application/rss+xml')][1]"));

                if (parser.updateContext ("/xhtml:html/xhtml:head/xhtml:link[@rel='alternate'][(@type='application/atom+xml') or (@type='application/rss+xml')][1]"))
                do
                {
                    std::string href = parser.evaluate ("@href");
                    if (href == "")
                    {
                        continue;
                    }

                    href = xml.buildURI(href, feed.source);

                    insertFeed.bind (1, href);
                    insertFeed.stepReset();

				    typeof(entry) n (context, href);
                    n.addMeta (mtCanonicalURI, href);
                    entry.linkTo (rLinksTo, n);

                    std::cerr << "h";
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
