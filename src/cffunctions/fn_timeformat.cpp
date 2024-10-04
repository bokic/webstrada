/**
 * @file fn_timeformat.cpp
 * @brief CFML timeformat() built-in.
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

cfvariant *cf_timeformat(const cfvariant *time, const cfvariant *mask) {
    if (!time) throw webstrada::exception("TimeFormat requires at least 1 argument");
    double days = getDaysOrThrow(time, "TimeFormat");
    string m = mask ? const_cast<cfvariant*>(mask)->toString() : "";
    string str = formatDateTime(days, m, ModeTime);
    auto *ret = new cfvariant(str);
    return ret;
}

} // namespace cfml
