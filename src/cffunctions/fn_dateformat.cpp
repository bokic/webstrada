/**
 * @file fn_dateformat.cpp
 * @brief CFML dateformat() built-in.
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

cfvariant *cf_dateformat(const cfvariant *date, const cfvariant *mask) {
    if (!date) throw webstrada::exception("DateFormat requires at least 1 argument");
    double days = getDaysOrThrow(date, "DateFormat");
    string m = mask ? const_cast<cfvariant*>(mask)->toString() : "";
    string str = formatDateTime(days, m, ModeDate);
    auto *ret = new cfvariant(str);
    return ret;
}

} // namespace cfml
