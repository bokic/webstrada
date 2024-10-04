/**
 * @file fn_urlsessionformat.cpp
 * @brief CFML urlsessionformat() built-in.
 */

#include "common.h"

#include "../cftags/common.h"
#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>
#include <webstrada/exceptions.h>
#include <webstrada/scope_store.h>
#include <webstrada/string.h>
#include <charconv>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <string>
#include <thread>
#include <chrono>
#include <vector>
#include <unistd.h>

using webstrada::cfvariant;
using webstrada::string;
using cfml::daysToTm;
using cfml::getIntValue;
using cfml::isTruthy;
using cfml::safe_to_std_string;
using cfml::variantToString;
using cfml::cfvariant_to_long;
using cfml::normalizeCharsetName;
using cfml::bytesToText;
using cfml::urlDecodeString;
using cfml::stringToBytes;
using cfml::getDaysOrThrow;
using cfml::tmToDays;
using cfml::cryptoHexDigits;

namespace cfml {

cfvariant *cf_urlsessionformat(const cfvariant *url) {
    if (!url) throw webstrada::exception("URLSessionFormat requires exactly 1 argument");
    string u = const_cast<cfvariant*>(url)->toString();

    // ColdFusion appends CFID/CFTOKEN to the URL only when a fresh session was
    // created in the current request (the client had no valid session cookie);
    // a request that already carried a valid session renders the URL unchanged.
    auto &sc = scope_context();
    if (sc.sessionEnabled && sc.sessionNewlyCreated && !sc.sessionId.empty()) {
        size_t colon = sc.sessionId.find(':');
        string cfid = (colon == std::string::npos) ? sc.sessionId.c_str() : sc.sessionId.substr(0, colon).c_str();
        string token = (colon == std::string::npos) ? "" : sc.sessionId.substr(colon + 1).c_str();
        bool hasQuery = u.contains('?');
        string suffix = hasQuery ? string("&CFID=") : string("?CFID=");
        suffix += cfid;
        suffix += "&CFTOKEN=";
        suffix += token;
        u += suffix;
    }

    auto *ret = new cfvariant(u);
    return ret;
}

} // namespace cfml
