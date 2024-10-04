/**
 * @file fn_datepart.cpp
 * @brief CFML datepart() built-in.
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

cfvariant *cf_datepart(const cfvariant *datepart, const cfvariant *date) {
    if (!datepart || !date) throw webstrada::exception("DatePart requires exactly 2 arguments");
    string dp = const_cast<cfvariant*>(datepart)->toString().trimmed();
    dp.toLower();
    double days = getDaysOrThrow(date, "DatePart");
    struct tm tm = daysToTm(days);

    int val = 0;
    if (dp.equals("yyyy")) {
        val = tm.tm_year + 1900;
    } else if (dp.equals("q")) {
        val = tm.tm_mon / 3 + 1;
    } else if (dp.equals("m")) {
        val = tm.tm_mon + 1;
    } else if (dp.equals("y")) {
        val = tm.tm_yday + 1;
    } else if (dp.equals("d")) {
        val = tm.tm_mday;
    } else if (dp.equals("w")) {
        val = tm.tm_wday + 1;
    } else if (dp.equals("ww")) {
        struct tm jan1 = tm; jan1.tm_mon = 0; jan1.tm_mday = 1; jan1.tm_hour = 0; jan1.tm_min = 0; jan1.tm_sec = 0;
        int jan1_wday = daysToTm(tmToDays(jan1)).tm_wday;
        val = (tm.tm_yday + jan1_wday) / 7 + 1;
    } else if (dp.equals("h")) {
        val = tm.tm_hour;
    } else if (dp.equals("n")) {
        val = tm.tm_min;
    } else if (dp.equals("s")) {
        val = tm.tm_sec;
    } else {
        throw webstrada::exception("DatePart: Invalid datepart '" + dp + "'");
    }

    auto *ret = new cfvariant(val);
    return ret;
}

} // namespace cfml
