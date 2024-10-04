/**
 * @file fn_daysinyear.cpp
 * @brief CFML daysinyear() built-in.
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

cfvariant *cf_daysinyear(const cfvariant *date) {
    if (!date) throw webstrada::exception("DaysInYear requires exactly 1 argument");
    double days = getDaysOrThrow(date, "DaysInYear");
    struct tm tm = daysToTm(days);
    auto *ret = new cfvariant(isLeapYear(tm.tm_year + 1900) ? 366 : 365);
    return ret;
}

} // namespace cfml
