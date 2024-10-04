/**
 * @file fn_dateconvert.cpp
 * @brief CFML dateconvert() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <chrono>
#include <ctime>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <map>
#include <string>

namespace cfml {

cfvariant *cf_dateconvert(const cfvariant *type, const cfvariant *date) {
    if (!type || !date) throw webstrada::exception("DateConvert requires exactly 2 arguments");
    string t = const_cast<cfvariant*>(type)->toString().trimmed();
    t.toLower();
    double days = getDaysOrThrow(date, "DateConvert");

    time_t now = time(nullptr);
    struct tm tm_local, tm_utc;
    localtime_r(&now, &tm_local);
    gmtime_r(&now, &tm_utc);

    double offset_days = tmToDays(tm_local) - tmToDays(tm_utc);

    if (t.equals("local2utc")) {
        days -= offset_days;
    } else if (t.equals("utc2local")) {
        days += offset_days;
    } else {
        throw webstrada::exception("DateConvert: Invalid conversion type '" + t + "'");
    }

    cfvariant res(cfvariant::DateTime);
    res.m_double = days;
    auto *ret = new cfvariant(res);
    return ret;
}

} // namespace cfml
