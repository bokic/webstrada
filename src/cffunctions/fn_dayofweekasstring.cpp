/**
 * @file fn_dayofweekasstring.cpp
 * @brief CFML dayofweekasstring() built-in.
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

cfvariant *cf_dayofweekasstring(const cfvariant *dayOfWeek, const cfvariant *locale) {
    if (!dayOfWeek) throw webstrada::exception("DayOfWeekAsString requires at least 1 argument");
    int day = getIntValue(*dayOfWeek);
    if (day < 1 || day > 7) throw webstrada::exception("DayOfWeekAsString: Only values from 1 to 7 are valid.");
    
    const char *days[] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };
    auto *ret = new cfvariant(days[day - 1]);
    return ret;
}

} // namespace cfml
