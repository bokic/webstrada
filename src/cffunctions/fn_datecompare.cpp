/**
 * @file fn_datecompare.cpp
 * @brief CFML datecompare() built-in.
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

cfvariant *cf_datecompare(const cfvariant *date1, const cfvariant *date2, const cfvariant *datePart) {
    if (!date1 || !date2) throw webstrada::exception("DateCompare requires at least 2 arguments");
    double d1 = getDaysOrThrow(date1, "DateCompare");
    double d2 = getDaysOrThrow(date2, "DateCompare");

    string dp = "";
    if (datePart && datePart->m_type != cfvariant::Null) {
        dp = const_cast<cfvariant*>(datePart)->toString().trimmed();
        dp.toLower();
    }

    struct tm tm1 = daysToTm(d1);
    struct tm tm2 = daysToTm(d2);

    int cmp = 0;
    if (dp.equals("yyyy")) {
        cmp = (tm1.tm_year < tm2.tm_year) ? -1 : ((tm1.tm_year > tm2.tm_year) ? 1 : 0);
    } else if (dp.equals("m")) {
        if (tm1.tm_year != tm2.tm_year) {
            cmp = (tm1.tm_year < tm2.tm_year) ? -1 : 1;
        } else {
            cmp = (tm1.tm_mon < tm2.tm_mon) ? -1 : ((tm1.tm_mon > tm2.tm_mon) ? 1 : 0);
        }
    } else if (dp.equals("d")) {
        if (tm1.tm_year != tm2.tm_year) cmp = (tm1.tm_year < tm2.tm_year) ? -1 : 1;
        else if (tm1.tm_mon != tm2.tm_mon) cmp = (tm1.tm_mon < tm2.tm_mon) ? -1 : 1;
        else cmp = (tm1.tm_mday < tm2.tm_mday) ? -1 : ((tm1.tm_mday > tm2.tm_mday) ? 1 : 0);
    } else if (dp.equals("h")) {
        if (tm1.tm_year != tm2.tm_year) cmp = (tm1.tm_year < tm2.tm_year) ? -1 : 1;
        else if (tm1.tm_mon != tm2.tm_mon) cmp = (tm1.tm_mon < tm2.tm_mon) ? -1 : 1;
        else if (tm1.tm_mday != tm2.tm_mday) cmp = (tm1.tm_mday < tm2.tm_mday) ? -1 : 1;
        else cmp = (tm1.tm_hour < tm2.tm_hour) ? -1 : ((tm1.tm_hour > tm2.tm_hour) ? 1 : 0);
    } else if (dp.equals("n")) {
        if (tm1.tm_year != tm2.tm_year) cmp = (tm1.tm_year < tm2.tm_year) ? -1 : 1;
        else if (tm1.tm_mon != tm2.tm_mon) cmp = (tm1.tm_mon < tm2.tm_mon) ? -1 : 1;
        else if (tm1.tm_mday != tm2.tm_mday) cmp = (tm1.tm_mday < tm2.tm_mday) ? -1 : 1;
        else if (tm1.tm_hour != tm2.tm_hour) cmp = (tm1.tm_hour < tm2.tm_hour) ? -1 : 1;
        else cmp = (tm1.tm_min < tm2.tm_min) ? -1 : ((tm1.tm_min > tm2.tm_min) ? 1 : 0);
    } else if (dp.isEmpty() || dp.equals("s")) {
        if (tm1.tm_year != tm2.tm_year) cmp = (tm1.tm_year < tm2.tm_year) ? -1 : 1;
        else if (tm1.tm_mon != tm2.tm_mon) cmp = (tm1.tm_mon < tm2.tm_mon) ? -1 : 1;
        else if (tm1.tm_mday != tm2.tm_mday) cmp = (tm1.tm_mday < tm2.tm_mday) ? -1 : 1;
        else if (tm1.tm_hour != tm2.tm_hour) cmp = (tm1.tm_hour < tm2.tm_hour) ? -1 : 1;
        else if (tm1.tm_min != tm2.tm_min) cmp = (tm1.tm_min < tm2.tm_min) ? -1 : 1;
        else cmp = (tm1.tm_sec < tm2.tm_sec) ? -1 : ((tm1.tm_sec > tm2.tm_sec) ? 1 : 0);
    } else {
        throw webstrada::exception("DateCompare: Invalid datePart '" + dp + "'");
    }

    auto *ret = new cfvariant(cmp);
    return ret;
}

} // namespace cfml
