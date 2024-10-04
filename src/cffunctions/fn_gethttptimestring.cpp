/**
 * @file fn_gethttptimestring.cpp
 * @brief CFML gethttptimestring() built-in.
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

cfvariant *cf_gethttptimestring(const cfvariant *date) {
    double days = 0.0;
    if (date && date->m_type != cfvariant::Null) {
        // CF interprets the date value as the server's local wall clock and
        // formats the instant in GMT (CreateDate(2026,6,1) on a UTC+2 server
        // prints "Sun, 31 May 2026 22:00:00 GMT"). Resolve the local offset
        // (DST-aware) via mktime, then render the resulting instant in UTC.
        days = getDaysOrThrow(date, "GetHttpTimeString");
        struct tm tm_local = daysToTm(days);
        tm_local.tm_isdst = -1;
        std::time_t epoch = mktime(&tm_local);
        struct tm tm_utc;
        gmtime_r(&epoch, &tm_utc);
        days = tmToDays(tm_utc);
    } else {
        std::time_t t = std::time(nullptr);
        struct tm tm_utc;
        gmtime_r(&t, &tm_utc);
        days = tmToDays(tm_utc);
    }
    struct tm tm = daysToTm(days);
    const char* wd[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    const char* mn[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    char buf[128];
    std::sprintf(buf, "%s, %02d %s %04d %02d:%02d:%02d GMT",
        wd[tm.tm_wday], tm.tm_mday, mn[tm.tm_mon], tm.tm_year + 1900,
        tm.tm_hour, tm.tm_min, tm.tm_sec);
    auto *ret = new cfvariant(buf);
    return ret;
}

} // namespace cfml
