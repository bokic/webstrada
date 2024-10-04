/**
 * @file fn_datediff.cpp
 * @brief CFML datediff() built-in.
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

cfvariant *cf_datediff(const cfvariant *datepart, const cfvariant *date1, const cfvariant *date2) {
    if (!datepart || !date1 || !date2) throw webstrada::exception("DateDiff requires exactly 3 arguments");
    string dp = const_cast<cfvariant*>(datepart)->toString().trimmed();
    dp.toLower();
    double d1 = getDaysOrThrow(date1, "DateDiff");
    double d2 = getDaysOrThrow(date2, "DateDiff");

    struct tm tm1 = daysToTm(d1);
    struct tm tm2 = daysToTm(d2);

    int diff = 0;
    if (dp.equals("yyyy")) {
        diff = tm2.tm_year - tm1.tm_year;
    } else if (dp.equals("q")) {
        diff = (tm2.tm_year - tm1.tm_year) * 4 + (tm2.tm_mon / 3 - tm1.tm_mon / 3);
    } else if (dp.equals("m")) {
        diff = (tm2.tm_year - tm1.tm_year) * 12 + (tm2.tm_mon - tm1.tm_mon);
    } else if (dp.equals("y") || dp.equals("d")) {
        diff = static_cast<int>(d2 - d1);
    } else if (dp.equals("w")) {
        diff = static_cast<int>((d2 - d1) / 7.0);
    } else if (dp.equals("ww")) {
        struct tm jan1_1 = tm1; jan1_1.tm_mon = 0; jan1_1.tm_mday = 1; jan1_1.tm_hour = 0; jan1_1.tm_min = 0; jan1_1.tm_sec = 0;
        int jan1_wday1 = daysToTm(tmToDays(jan1_1)).tm_wday;
        int w1 = (tm1.tm_yday + jan1_wday1) / 7;

        struct tm jan1_2 = tm2; jan1_2.tm_mon = 0; jan1_2.tm_mday = 1; jan1_2.tm_hour = 0; jan1_2.tm_min = 0; jan1_2.tm_sec = 0;
        int jan1_wday2 = daysToTm(tmToDays(jan1_2)).tm_wday;
        int w2 = (tm2.tm_yday + jan1_wday2) / 7;

        diff = (tm2.tm_year - tm1.tm_year) * 52 + (w2 - w1);
    } else if (dp.equals("h")) {
        diff = static_cast<int>((d2 - d1) * 24.0);
    } else if (dp.equals("n")) {
        diff = static_cast<int>((d2 - d1) * 1440.0);
    } else if (dp.equals("s")) {
        diff = static_cast<int>((d2 - d1) * 86400.0);
    } else {
        throw webstrada::exception("DateDiff: Invalid datepart '" + dp + "'");
    }

    auto *ret = new cfvariant(diff);
    return ret;
}

} // namespace cfml
