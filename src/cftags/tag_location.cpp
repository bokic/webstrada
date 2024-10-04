/**
 * @file tag_location.cpp
 * @brief <cflocation> runtime (response_redirect).
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>
#include <webstrada/config.h>
#include <webstrada/exceptions.h>
#include <webstrada/string.h>

#include <sys/stat.h>
#include <unistd.h>
#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace cfml {

void response_redirect(const cfvariant *url, const cfvariant *addToken,
                             const cfvariant *statusCode)
{
    auto &r = response();

    int code = statusCode ? cfvariant_to_int(statusCode) : 302;
    if (code < 300 || code > 307) {
        throw webstrada::exception("Attribute validation error in the cflocation tag. Value of statuscode must be between 300 and 307.");
    }

    string urlStr = url ? stripCRLF(variantToString(*url)) : string();
    urlStr = encodeControlChars(urlStr);

    bool addTok = addToken ? cfmlBoolean(addToken, true) : true;
    if (addTok && !urlContainsCFTokens(urlStr)) {
        auto &sc = scope_context();
        if (sc.sessionEnabled && sc.sessionNewlyCreated && !sc.sessionId.empty()) {
            size_t colon = sc.sessionId.find(':');
            string cfid = (colon == std::string::npos)
                ? sc.sessionId.c_str() : sc.sessionId.substr(0, colon).c_str();
            string token = (colon == std::string::npos)
                ? "" : sc.sessionId.substr(colon + 1).c_str();
            string suffix = urlStr.contains('?')
                ? string("&CFID=") : string("?CFID=");
            suffix += cfid;
            suffix += "&CFTOKEN=";
            suffix += token;
            urlStr += suffix;
        }
    }

    if (!r.committed) {
        r.statusCode = code;
        responseSetHeader(r, "location", urlStr);
        responseSetHeader(r, "Cache-Control", "no-cache");
        responseSetHeader(r, "Pragma", "no-cache");
    }
    throw webstrada::abort_exception();
}

} // namespace cfml
