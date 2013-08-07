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

#if !defined(FEED_DOWNLOAD_H)
#define FEED_DOWNLOAD_H

#include <feed/handler.h>
#include <curl/curl.h>
#include <set>
#include <map>
#include <sstream>
#include <cstring>

namespace feed
{
    class download : public handler
    {
    public:
        download(configuration &pConfiguration, const bool &pEnabled)
            : handler(pConfiguration, stDownload, pEnabled)
        {
            if (curl_global_init(CURL_GLOBAL_DEFAULT))
            {
                throw exceptionCURL("curl_global_init()");
            }

            curl = curl_multi_init();
            if (curl == 0)
            {
                throw exceptionCURL("curl_multi_init()");
            }

            share = curl_share_init();
            if (share == 0)
            {
                throw exceptionCURL("curl_share_init()");
            }

            if (curl_share_setopt(share, CURLSHOPT_SHARE, CURL_LOCK_DATA_COOKIE) != CURLSHE_OK)
            {
                throw exceptionCURL("curl_share_setopt(...,CURL_LOCK_DATA_COOKIE)");
            }

            if (curl_share_setopt(share, CURLSHOPT_SHARE, CURL_LOCK_DATA_DNS) != CURLSHE_OK)
            {
                throw exceptionCURL("curl_share_setopt(...,CURL_LOCK_DATA_DNS)");
            }

            if (curl_share_setopt(share, CURLSHOPT_SHARE, CURL_LOCK_DATA_SSL_SESSION) != CURLSHE_OK)
            {
                throw exceptionCURL("curl_share_setopt(...,CURL_LOCK_DATA_SSL_SESSION)");
            }
        }

        ~download(void)
        {
            if (curl_share_cleanup (share) != CURLSHE_OK)
            {
                throw exceptionCURL("curl_share_cleanup()");
            }

            if (curl_multi_cleanup (curl) != CURLM_OK)
            {
                throw exceptionCURL("curl_multi_cleanup()");
            }

            curl_global_cleanup();
        }

        virtual bool handle
            (const enum servicetype &st,
             feed::feed &feed)
        {
            if (!enabled)
            {
                return false;
            }

            if (st != stDownload)
            {
                return false;
            }
            std::cerr << "D";

            return queueTransfer (feed.source);
        }

        bool queueTransfer
            (const std::string &uri,
             const std::string &payload = "",
             const enum servicetype &st = stDownload)
        {
            if (!enabled)
            {
                return false;
            }

            new transfer (uri, *this, payload, st);

            return true;
        }

        bool process(void)
        {
            int numfds = 0;
#if 0
            if (curl_multi_wait (curl, 0, 0, 5000, &numfds) != CURLM_OK)
            {
                throw exceptionCURL("curl_multi_wait()");
            }
#else
            if (curl_multi_perform (curl, &numfds) != CURLM_OK)
            {
                throw exceptionCURL("curl_multi_perform()");
            }
#endif

            int msglen;

            do
            {
                CURLMsg *msg = curl_multi_info_read (curl, &msglen);
                if (msg)
                {
                    switch (msg->msg)
                    {
                        case CURLMSG_DONE:
                            delete transfermap[msg->easy_handle];
                            break;
                        case CURLMSG_NONE:
                        case CURLMSG_LAST:
                            break;
                    }
                }
            } while (msglen > 0);

            return !activeTransfers();
        }

        bool activeTransfers (void) const
        {
            return transfers.size() != 0;
        }

        class data
        {
        public:
            data(int pDownloadID, download &download)
                : downloadID(pDownloadID)
            {
                sqlite::statement stmt
                    ("select data, uri from download where id = ?1",
                     download.context.sql);
                stmt.bind (1, downloadID);
                if (stmt.step())
                {
                    stmt.get(0, content);
                    stmt.get(1, filename);
                }
                else
                {
                    throw exception("tried to retrieve data from nonexistent download");
                }
            }

            std::string content;
            std::string filename;

        protected:
            int downloadID;
        };

        data retrieve
            (int downloadID)
        {
            return data(downloadID, *this);
        }

        data retrieve
            (const std::string &uri)
        {
            sqlite::statement stmt
                ("select id from download where uri = ?1 and data is not null and completiontime is not null order by completiontime desc limit 1",
                 context.sql);

            stmt.bind (1, uri);
            stmt.step ();
            if (!stmt.row)
            {
                throw exception("tried to retrieve data from nonexistent download at " + uri);
            }

            int downloadID;
            stmt.get (0, downloadID);

            return data(downloadID, *this);
        }

        data retrieve
            (const std::string &uri,
             const std::string &payload)
        {
            sqlite::statement stmt
                ("select id from download where uri = ?1 and payload = ?2 and data is not null and completiontime is not null order by completiontime desc limit 1",
                 context.sql);

            stmt.bind (1, uri);
            stmt.bind (2, payload);
            stmt.step ();
            if (!stmt.row)
            {
                throw exception("tried to retrieve data from nonexistent download at " + uri + " with payload " + payload);
            }

            int downloadID;
            stmt.get (0, downloadID);

            return data(downloadID, *this);
        }

    protected:
        class transfer
        {
        public:
            transfer(const std::string &pUri, download &pDownload, const std::string &pPayload = "", const enum servicetype &pService = stDownload)
                : uri(pUri), download(pDownload), content_length(-1), blob(0), offset(0L), payload(pPayload)
            {
                {
                    sqlite::statement stmt
                        ("insert into download (uri, requesttime, payload, sid) values (?1, julianday('now'), ?2, ?3)",
                         download.context.sql);
                    stmt.bind (1, uri);
                    if (payload != "")
                    {
                        stmt.bind (2, payload);
                    }
                    int serviceID = (int)pService;
                    stmt.bind (3, serviceID);
                    stmt.step ();

                    id = sqlite3_last_insert_rowid(download.context.database);
                }

                handle = curl_easy_init();
                if (handle == 0)
                {
                    throw exceptionCURL("curl_easy_init()");
                }

                if (curl_easy_setopt(handle, CURLOPT_URL, uri.c_str()) != CURLE_OK)
                {
                    throw exceptionCURL("curl_easy_setopt(...,CURLOPT_URL,...)");
                }

                if (curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, dataCallbackS) != CURLE_OK)
                {
                    throw exceptionCURL("curl_easy_setopt(...,CURLOPT_WRITEFUNCTION,...)");
                }

                if (curl_easy_setopt(handle, CURLOPT_WRITEDATA, this) != CURLE_OK)
                {
                    throw exceptionCURL("curl_easy_setopt(...,CURLOPT_WRITEDATA,...)");
                }

                if (payload != "")
                {
                    if (curl_easy_setopt(handle, CURLOPT_READFUNCTION, dataInputCallbackS) != CURLE_OK)
                    {
                        throw exceptionCURL("curl_easy_setopt(...,CURLOPT_READFUNCTION,...)");
                    }

                    if (curl_easy_setopt(handle, CURLOPT_READDATA, this) != CURLE_OK)
                    {
                        throw exceptionCURL("curl_easy_setopt(...,CURLOPT_READDATA,...)");
                    }
                }

                if (curl_easy_setopt(handle, CURLOPT_SHARE, download.share) != CURLSHE_OK)
                {
                    throw exceptionCURL("curl_easy_setopt(...,CURLOPT_SHARE,...)");
                }

                if (curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1) != CURLSHE_OK)
                {
                    throw exceptionCURL("curl_easy_setopt(...,CURLOPT_FOLLOWLOCATION,...)");
                }

                if ((download.context.proxy != "")
                 && (curl_easy_setopt(handle, CURLOPT_PROXY, download.context.proxy.c_str()) != CURLE_OK))
                {
                    throw exceptionCURL("curl_easy_setopt(...,CURLOPT_PROXY,...)");
                }

                if ((download.context.userAgent != "")
                 && (curl_easy_setopt(handle, CURLOPT_USERAGENT, download.context.userAgent.c_str()) != CURLE_OK))
                {
                    throw exceptionCURL("curl_easy_setopt(...,CURLOPT_USERAGENT,...)");
                }

                if (curl_multi_add_handle(download.curl, handle) != CURLM_OK)
                {
                    throw exceptionCURL("curl_multi_add_handle()");
                }

                download.transfers.insert(this);
                download.transfermap[handle] = this;
            }

            ~transfer(void)
            {
                sqlite::statement stmt
                    ("update download set completiontime=julianday('now') where id=?1",
                     download.context.sql);

                stmt.bind (1, id);
                stmt.step();

                if (blob && (sqlite3_blob_close (blob) != SQLITE_OK))
                {
                    throw exceptionSQLITE("sqlite3_blob_close()", download.context);
                }

                if (curl_multi_remove_handle(download.curl, handle) != CURLM_OK)
                {
                    throw exceptionCURL("curl_multi_remove_handle()");
                }

                curl_easy_cleanup(handle);

                download.transfermap.erase(handle);
                download.transfers.erase(this);
            }

        protected:
            static size_t dataCallbackS (char *data, size_t size, size_t nmemb, void *aux)
            {
                transfer *a = (transfer *)aux;
                return a->dataCallback (data, size, nmemb);
            }

            static size_t dataInputCallbackS (void *ptr, size_t size, size_t nmemb, void *aux)
            {
                transfer *a = (transfer *)aux;
                return a->dataInputCallback (ptr, size, nmemb);
            }

            size_t dataCallback (char *data, size_t size, size_t nmemb)
            {
                double len;

                curl_easy_getinfo(handle, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &len);
                if (len != content_length)
                {
                    content_length = len;
                    if (len >= 0)
                    {
                        {
                            sqlite::statement stmt
                                ("update download set data = zeroblob(?2), length=?2 where id=?1",
                                 download.context.sql);
                            stmt.bind (1, id);
                            stmt.bind (2, len);
                            stmt.step ();
                        }

                        if (sqlite3_blob_open
                                (download.context.database,
                                 "main", "download", "data",
                                 id, 1, &blob) != SQLITE_OK)
                        {
                            throw exceptionSQLITE("sqlite3_blob_open()", download.context);
                        }

                        offset = 0L;
                    }
                }

                std::cerr << ".";
                if (blob)
                {
                retry:
                    int r = sqlite3_blob_write(blob, (void*)data, size*nmemb, offset);
                    switch (r)
                    {
                        case SQLITE_DONE:
                        case SQLITE_OK:
                            break;
                        case SQLITE_ABORT:
                            /* need to reopen the blob handle because it's been closed for some reason */
                            if ((sqlite3_blob_close (blob) != SQLITE_OK))
                            {
                                throw exceptionSQLITE("sqlite3_blob_close()", download.context);
                            }
                            if (sqlite3_blob_open
                                    (download.context.database,
                                     "main", "download", "data",
                                     id, 1, &blob) != SQLITE_OK)
                            {
                                throw exceptionSQLITE("sqlite3_blob_open()", download.context);
                            }
                            goto retry;
                        default:
                        {
                            std::stringstream st;
                            st << "sqlite3_blob_write(): returned code #" << r;
                            std::string stv(st.str());
                            throw exceptionSQLITE(stv, download.context);
                        }
                    }
                    offset += size * nmemb;
                }
                else
                {
                    std::string newdata((const char*)data, size*nmemb);

                    sqlite::statement stmt
                        ("update download set data = coalesce(data,'') || ?2, length = coalesce(length,0) + ?3 where id = ?1",
                         download.context.sql);
                    stmt.bind (1, id);
                    stmt.bind (2, newdata);
                    stmt.bind (3, newdata.size());
                    stmt.step ();
                }

                return size * nmemb;
            }

            size_t dataInputCallback (void *data, size_t size, size_t nmemb)
            {
                size_t byteCount = std::min(payload.size(), (size * nmemb));
                if (payload.size() == 0)
                {
                    return 0;
                }

                std::memcpy (data, payload.data(), byteCount);

                payload.erase (0, byteCount);

                return byteCount;
            }

            const std::string uri;
            std::string payload;

            int id;
            download &download;
            CURL *handle;
            sqlite3_blob *blob;
            double content_length;
            size_t offset;
        };

        CURLM *curl;
        CURLSH *share;
        std::set<transfer*> transfers;
        std::map<CURL*,transfer*> transfermap;
    };
};

#endif
