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

#if !defined(FEED_XML_H)
#define FEED_XML_H

#include <feed/download.h>
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <libxml/uri.h>
#include <set>

namespace feed
{
    class xml
    {
    public:
        xml(configuration &pConfiguration, download &pDownload)
            : configuration(pConfiguration), download(pDownload)
        {
            LIBXML_TEST_VERSION
        }

        ~xml(void)
        {
            xmlCleanupParser();
        }

        class parser
        {
        public:
            parser(xml &pXml,
                   const std::string &data,
                   const std::string &filename)
                : xml (pXml),
                  document (xmlReadMemory (data.data(), data.size(), filename.c_str(), NULL, 0))
            {
                if (document == 0)
                {
                    throw exception("failed to parse xml file " + filename);
                }

                xpathContext = xmlXPathNewContext(document);
                if (xpathContext == 0)
                {
                    xmlFreeDoc(document); 
                    throw exception("failed to create XPath context");
                }

                if (xmlXPathRegisterNs
                        (xpathContext,
                         (const xmlChar *)"atom",
                         (const xmlChar *)"http://www.w3.org/2005/Atom") != 0)
                {
                    xmlFreeDoc(document); 
                    xmlXPathFreeContext(xpathContext); 
                    throw exception("failed to register namespace: atom");
                }

                if (xmlXPathRegisterNs
                        (xpathContext,
                         (const xmlChar *)"xhtml",
                         (const xmlChar *)"http://www.w3.org/1999/xhtml") != 0)
                {
                    xmlFreeDoc(document); 
                    xmlXPathFreeContext(xpathContext); 
                    throw exception("failed to register namespace: xhtml");
                }

                xml.parsers.insert (this);
            }

            ~parser(void)
            {
                xml.parsers.erase (this);

                xmlXPathFreeContext(xpathContext);
                xmlFreeDoc(document);
            }

            const std::string evaluate
                (const std::string &expression)
            {
                xmlXPathObjectPtr xpathObject = lookup (expression);

                const xmlChar *resultc = xmlXPathCastNodeSetToString(xpathObject->nodesetval);
                const std::string result((const char *)resultc);
                free((void*)resultc);

                xmlXPathFreeObject(xpathObject);

                return result;
            }

            const std::string evaluateToFragment
                (const std::string &expression)
            {
                xmlXPathObjectPtr xpathObject = lookup (expression);

                xmlBufferPtr buffer = xmlBufferCreate();
                if (buffer == 0)
                {
                    throw exception("failed to create xmlBufferPtr");
                }

                xmlNodeSetPtr nodeset = xpathObject->nodesetval;

                if ((nodeset == 0) || (nodeset->nodeNr < 1) || (nodeset->nodeTab == 0))
                {
                    return "";
                }

                if (xmlNodeDump (buffer, document, nodeset->nodeTab[0], 0, 0) == -1)
                {
                    throw exception("could not generate XML fragment");
                }

                const std::string rv((const char*)buffer->content, buffer->size);

                xmlBufferFree (buffer);

                return rv;
            }

            bool updateContext
                (const std::string &expression)
            {
                xmlXPathObjectPtr xpathObject = lookup (expression);
                xmlNodeSetPtr nodeset = xpathObject->nodesetval;

                if ((nodeset == 0) || (nodeset->nodeNr < 1) || (nodeset->nodeTab == 0))
                {
                    return false;
                }

                if (nodeset->nodeNr != 1)
                {
                    return false;
                }

                xmlNodePtr node = nodeset->nodeTab[0];

                if (node == 0)
                {
                    return false;
                }

                xpathContext->node = node;

                xmlXPathFreeObject (xpathObject);

                return true;
            }

            xmlDocPtr document;
            xmlXPathContextPtr xpathContext;

        protected:
            xml &xml;

            xmlXPathObjectPtr lookup (const std::string &expression)
            {
                xmlXPathObjectPtr xpathObject = xmlXPathEvalExpression
                    ((const xmlChar *)expression.c_str(),
                     xpathContext);
                if (xpathObject == 0)
                {
                    throw exception("failed to evaluate XPath expression: " + expression);
                }

                return xpathObject;
            }
        };

        parser parse
            (const std::string &data,
             const std::string &filename)
        {
            return parser(*this, data, filename);
        }

        parser parse
            (const int downloadID)
        {
            download::data data = download.retrieve (downloadID);
            return parse(data.content, data.filename);
        }

        parser parse
            (const std::string &uri)
        {
            download::data data = download.retrieve (uri);
            return parse(data.content, data.filename);
        }

        bool parseURI
            (const std::string &uri,
             std::string &scheme,
             std::string &opaque,
             std::string &authority,
             std::string &server,
             std::string &user,
             int &port,
             std::string &path,
             std::string &query,
             std::string &fragment)
        {
            xmlURIPtr output = xmlParseURI (uri.c_str());
            if (output == 0)
            {
                return false;
            }

            scheme = output->scheme ? output->scheme : "";
            opaque = output->opaque ? output->opaque : "";
            authority = output->authority ? output->authority : "";
            server = output->server ? output->server : "";
            user = output->user ? output->user : "";
            port = output->port ? output->port : -1;
            path = output->query ? output->query : "";
            fragment = output->fragment ? output->fragment : "";

            xmlFreeURI (output);

            return true;
        }

    protected:
        configuration &configuration;
        download &download;
        std::set<parser*> parsers;
    };
};

#endif
