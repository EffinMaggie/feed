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

#if !defined(DIVE_CONSTANTS_H)
#define DIVE_CONSTANTS_H

namespace feed
{
    enum optiontype
    {
        otSchemaVersion   = -1,
        otProxy           = 0,
        otUserAgent       = 1
    };

    enum servicetype
    {
        stAtom            = 0,
        stRSS             = 1,
        stWhois           = 2,
        stDNS             = 3,
        stICal            = 4,
        stXHTML           = 5,
        stHTML            = 6,
        stDownload        = 7
    };

    enum metatype
    {
        mtContentID       = 0,
        mtTitle           = 1,
        mtSubtitle        = 2,
        mtLanguage        = 3,
        mtUpdated         = 4,
        mtPublished       = 5,
        mtAbstract        = 6,
        mtContent         = 7,
        mtContentMIME     = 8,
        mtContentEncoding = 9,
        mtCanonicalURI    = 10,
        mtSourceFeed      = 11,
        mtAuthorName      = 12,
        mtAuthorEmail     = 13,
        mtRead            = 14,
        mtMarked          = 15
    };

    enum commandtype
    {
        ctUpdateFeed      = 0,
        ctQuery           = 1,
        ctQuit            = 2,
        ctDownloadComplete= 3
    };

    enum protocol
    {
        pIPv4             = 0,
        pIPv6             = 1
    };

    enum relation
    {
        rAlternate        = 0,
        rUses             = 1,
        rLinksTo          = 2,
        rPaymentOf        = 3,
        rAuthor           = 4,
        rContributor      = 5,
        rEditor           = 6,
        rWebmaster        = 7,
        rEncloses         = 8
    };
};

#endif
