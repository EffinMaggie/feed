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

#if !defined(FEED_ATOM_H)
#define FEED_ATOM_H

#include <feed/handler.h>
#include <feed/entry.h>
#include <feed/xml.h>
#include <feed/person.h>

namespace feed
{
    class atom : public handler
    {
    public:
        atom(configuration &pConfiguration, const bool &pEnabled, xml &pXml)
            : handler(pConfiguration, stAtom, pEnabled), xml(pXml)
        {}

        virtual bool handle
            (const enum servicetype &st,
             feed::feed &feed)
        {
            if (!enabled)
            {
                return false;
            }

            if (st != stAtom)
            {
                return false;
            }
            std::cerr << "A";

            try
            {
                xml::parser parser = xml.parse (feed.source);

                //std::string title = parser.evaluate("/atom:feed/atom:title");
                if (parser.updateContext ("/atom:feed"))
                {
                    std::string xid = parser.evaluate ("atom:id");
                    if (xid != "")
                    {
                        entry entry(context, xid);

                        entry.addMeta (mtSourceFeed, feed.source);

                        std::string value;

                        if ((value = parser.evaluate("atom:title")) != "")
                        {
                            entry.addMeta (mtTitle, value);
                        }
                        if ((value = parser.evaluate("atom:subtitle")) != "")
                        {
                            entry.addMeta (mtSubtitle, value);
                        }
                        if ((value = parser.evaluate("atom:updated")) != "")
                        {
                            entry.addMeta (mtUpdated, value);
                        }

                        parsePersons (parser, entry);
                    }
                }

                if (parser.updateContext ("/atom:feed/atom:entry[1]"))
                do
                {
                    std::string xid = parser.evaluate ("atom:id");
                    if (xid == "")
                    {
                        continue;
                    }

                    entry entry(context, xid);

                    entry.addMeta (mtSourceFeed, feed.source);

                    std::string value;
                    std::string contentType;

                    if ((value = parser.evaluate("atom:title")) != "")
                    {
                        entry.addMeta (mtTitle, value);
                    }
                    if ((value = parser.evaluate("atom:summary")) != "")
                    {
                        entry.addMeta (mtAbstract, value);
                    }
                    if ((value = parser.evaluate("atom:updated")) != "")
                    {
                        entry.addMeta (mtUpdated, value);
                    }
                    if ((value = parser.evaluate("atom:published")) != "")
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
                    if ((value = parser.evaluateToFragment("atom:content/node() | atom:content/text()")) != "")
                    {
                        entry.addMeta (mtContent, value);
                    }

                    parsePersons (parser, entry);

                    std::cerr << "a";
                }
                while (parser.updateContext ("following-sibling::atom:entry[1]"));
            }
            catch (exception &e)
            {
                // std::cerr << "EXCEPTION: " << e.string << "\n";
                return true;
            }

            return true;
        }

    protected:
        void parsePersons (xml::parser &parser, entry &entry)
        {
            if (parser.updateContext ("atom:author[1]"))
            {
                do
                {
                    person person = parsePersonData (parser);
                    person.relatedTo (rAuthor, entry);
                }
                while (parser.updateContext ("following-sibling::atom:author[1]"));

                parser.updateContext ("..");
            }
            if (parser.updateContext ("atom:contributor[1]"))
            {
                do
                {
                    person person = parsePersonData (parser);
                    person.relatedTo (rContributor, entry);
                }
                while (parser.updateContext ("following-sibling::atom:contributor[1]"));

                parser.updateContext ("..");
            }
        }

        person parsePersonData (xml::parser &parser)
        {
            const std::string email = parser.evaluate("atom:email");
            const std::string name  = parser.evaluate("atom:name");
            const std::string uri   = parser.evaluate("atom:uri");
            if (email != "")
            {
                person person (context, email, mtAuthorEmail);
                if (name != "") { person.addMeta (mtAuthorName,  name); }
                if (uri  != "") { person.addMeta (mtCanonicalURI, uri); }
                return person;
            } 
            else if (name != "")
            {
                person person (context, name, mtAuthorName);
                if (uri  != "") { person.addMeta (mtCanonicalURI, uri); }
                return person;
            }
            else if (name != "")
            {
                person person (context, uri, mtCanonicalURI);
                return person;
            }

            person person (context, "Anonymous", mtAuthorName);
            return person;
        }

        xml &xml;
    };
};

#endif
