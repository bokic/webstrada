/**
 * @file fn_isonline.cpp
 * @brief CFML isonline() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>
#include <webstrada/exceptions.h>
#include <webstrada/string.h>
#include <curl/curl.h>
#include <string>

namespace cfml {

cfvariant *cf_isonline(const cfvariant *value) {
    auto *ret = new cfvariant(cfvariant::Boolean);
    ret->m_bool = false;
    if (!value) return ret;
    webstrada::string url = const_cast<cfvariant*>(value)->toString();
    const char *u = url.constData();
    if (!u || !*u) return ret;

    CURL *curl = curl_easy_init();
    if (!curl) return ret;
    curl_easy_setopt(curl, CURLOPT_URL, u);
    curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);      // HEAD request
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    // Discard any response body.
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, +[](char *, size_t size, size_t nmemb, void *) {
        return size * nmemb;
    });
    CURLcode res = curl_easy_perform(curl);
    long code = 0;
    if (res == CURLE_OK) curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    curl_easy_cleanup(curl);
    // Online = the request completed (any HTTP status, including 4xx/5xx).
    ret->m_bool = (res == CURLE_OK);
    return ret;
}

} // namespace cfml
