/**
 * @file fn_datetimeformat.cpp
 * @brief CFML datetimeformat() built-in.
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

cfvariant *cf_datetimeformat(const cfvariant *date, const cfvariant *mask) {
    if (!date) throw webstrada::exception("DateTimeFormat requires at least 1 argument");
    double days = getDaysOrThrow(date, "DateTimeFormat");
    string m = mask ? const_cast<cfvariant*>(mask)->toString() : "";
    string str = formatDateTime(days, m, ModeDateTime);
    auto *ret = new cfvariant(str);
    return ret;
}

} // namespace cfml
