/**
 * @file fn_dateadd.cpp
 * @brief CFML dateadd() built-in.
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

cfvariant *cf_dateadd(const cfvariant *datepart, const cfvariant *number, const cfvariant *date) {
    if (!datepart || !number || !date) throw webstrada::exception("DateAdd requires exactly 3 arguments");
    string dp = const_cast<cfvariant*>(datepart)->toString().trimmed();
    dp.toLower();
    int num = getIntValue(*number);
    double days = getDaysOrThrow(date, "DateAdd");

    if (dp.equals("yyyy")) {
        struct tm tm = daysToTm(days);
        tm.tm_year += num;
        normalizeTm(tm);
        days = tmToDays(tm);
    } else if (dp.equals("q")) {
        struct tm tm = daysToTm(days);
        tm.tm_mon += num * 3;
        normalizeTm(tm);
        days = tmToDays(tm);
    } else if (dp.equals("m")) {
        struct tm tm = daysToTm(days);
        tm.tm_mon += num;
        normalizeTm(tm);
        days = tmToDays(tm);
    } else if (dp.equals("y") || dp.equals("d") || dp.equals("w")) {
        days += num;
    } else if (dp.equals("ww")) {
        days += num * 7.0;
    } else if (dp.equals("h")) {
        days += num / 24.0;
    } else if (dp.equals("n")) {
        days += num / 1440.0;
    } else if (dp.equals("s")) {
        days += num / 86400.0;
    } else if (dp.equals("l")) {
        days += num / 86400000.0;
    } else {
        throw webstrada::exception("DateAdd: Invalid datepart '" + dp + "'");
    }

    cfvariant res(cfvariant::DateTime);
    res.m_double = days;
    auto *ret = new cfvariant(res);
    return ret;
}

} // namespace cfml
