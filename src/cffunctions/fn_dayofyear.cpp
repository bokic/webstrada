/**
 * @file fn_dayofyear.cpp
 * @brief CFML dayofyear() built-in.
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

cfvariant *cf_dayofyear(const cfvariant *date) {
    if (!date) throw webstrada::exception("DayOfYear requires exactly 1 argument");
    double days = getDaysOrThrow(date, "DayOfYear");
    struct tm tm = daysToTm(days);
    auto *ret = new cfvariant(tm.tm_yday + 1);
    return ret;
}

} // namespace cfml
