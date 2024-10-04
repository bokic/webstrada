/**
 * @file fn_daysinmonth.cpp
 * @brief CFML daysinmonth() built-in.
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

cfvariant *cf_daysinmonth(const cfvariant *date) {
    if (!date) throw webstrada::exception("DaysInMonth requires exactly 1 argument");
    double days = getDaysOrThrow(date, "DaysInMonth");
    struct tm tm = daysToTm(days);
    auto *ret = new cfvariant(getDaysInMonthVal(tm.tm_year + 1900, tm.tm_mon));
    return ret;
}

} // namespace cfml
