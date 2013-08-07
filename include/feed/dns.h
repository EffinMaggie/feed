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

#if !defined(FEED_DNS_H)
#define FEED_DNS_H

#include <feed/handler.h>
#include <feed/entry.h>
#include <feed/xml.h>
#include <netdb.h>
#include <arpa/inet.h>

namespace feed
{
    class dns : public handler
    {
    public:
        dns(configuration &pConfiguration, const bool &pEnabled, xml &pXml)
            : handler(pConfiguration, stDNS, pEnabled), xml(pXml),
              insertDNSIPv4("insert or ignore into dns (hostname, pid, address) values (?1, 0, ?2)", context.sql),
              insertDNSIPv6("insert or ignore into dns (hostname, pid, address) values (?1, 1, ?2)", context.sql)
        {}

        virtual bool handle
            (const enum servicetype &st,
             feed::feed &feed)
        {
            if (!enabled)
            {
                return false;
            }

            if (st != stDNS)
            {
                return false;
            }
            std::cerr << "D";

            std::string scheme;
            std::string opaque;
            std::string authority;
            std::string server;
            std::string user;
            int port;
            std::string path;
            std::string query;
            std::string fragment;

            if (xml.parseURI
                    (feed.source,
                     scheme, opaque, authority, server,
                     user, port, path, query, fragment) == false)
            {
                std::cerr << "could not parse URI: " << feed.source << "\n";
                return true;
            }

            struct addrinfo hints = { 0 };
            struct addrinfo *result;

            hints.ai_family = AF_UNSPEC;
            hints.ai_socktype = SOCK_DGRAM;
            hints.ai_flags = 0;
            hints.ai_protocol = 0;

            if ((getaddrinfo(server.c_str(), 0, &hints, &result)) != 0)
            {
                std::cerr << "error resolving hostname" << server << "\n";
                return true;
            }

            insertDNSIPv4.bind (1, server);
            insertDNSIPv6.bind (1, server);

            for (struct addrinfo *cur = result; cur != 0; cur = cur->ai_next)
            {
                switch (cur->ai_family)
                {
                    case AF_INET:
                    {
                        char address[INET_ADDRSTRLEN];
                        const char *r = inet_ntop(cur->ai_family, &((struct sockaddr_in *)cur->ai_addr)->sin_addr, address, INET_ADDRSTRLEN);
                        if (r != 0)
                        {
                            insertDNSIPv4.bind (2, std::string(r));
                            insertDNSIPv4.stepReset ();
                        }
                    }
                        break;
                    case AF_INET6:
                    {
                        char address[INET6_ADDRSTRLEN];
                        const char *r = inet_ntop(cur->ai_family, &((struct sockaddr_in6 *)cur->ai_addr)->sin6_addr, address, INET6_ADDRSTRLEN);
                        if (r != 0)
                        {
                            insertDNSIPv6.bind (2, std::string(r));
                            insertDNSIPv6.stepReset ();
                        }
                    }
                        break;
                }
            }

            freeaddrinfo (result);

            return true;
        }

    protected:
        sqlite::statement insertDNSIPv4;
        sqlite::statement insertDNSIPv6;
        xml &xml;
    };
};

#endif
