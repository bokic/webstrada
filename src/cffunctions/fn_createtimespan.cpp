/**
 * @file fn_createtimespan.cpp
 * @brief CFML createtimespan() built-in.
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

cfvariant *cf_createtimespan(const cfvariant *days, const cfvariant *hours, const cfvariant *minutes, const cfvariant *seconds) {
    if (!days || !hours || !minutes || !seconds) throw webstrada::exception("CreateTimeSpan requires exactly 4 arguments");
    double d = getDoubleValue(*days);
    double h = getDoubleValue(*hours);
    double m = getDoubleValue(*minutes);
    double s = getDoubleValue(*seconds);
    cfvariant res(cfvariant::Float);
    res.m_double = d + h/24.0 + m/1440.0 + s/86400.0;
    auto *ret = new cfvariant(res);
    return ret;
}

} // namespace cfml
