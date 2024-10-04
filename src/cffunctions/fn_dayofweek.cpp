/**
 * @file fn_dayofweek.cpp
 * @brief CFML dayofweek() built-in.
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

cfvariant *cf_dayofweek(const cfvariant *date) {
    if (!date) throw webstrada::exception("DayOfWeek requires exactly 1 argument");
    double days = getDaysOrThrow(date, "DayOfWeek");
    struct tm tm = daysToTm(days);
    auto *ret = new cfvariant(tm.tm_wday + 1);
    return ret;
}

} // namespace cfml
