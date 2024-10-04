/**
 * @file fn_createtime.cpp
 * @brief CFML createtime() built-in.
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

cfvariant *cf_createtime(const cfvariant *hrVal, const cfvariant *minVal, const cfvariant *secVal) {
    if (!hrVal || !minVal || !secVal) throw webstrada::exception("CreateTime requires exactly 3 arguments");
    int hr = getIntValue(*hrVal);
    int min = getIntValue(*minVal);
    int sec = getIntValue(*secVal);

    struct tm tm;
    memset(&tm, 0, sizeof(tm));
    tm.tm_year = 1899 - 1900;
    tm.tm_mon = 12 - 1;
    tm.tm_mday = 30;
    tm.tm_hour = hr;
    tm.tm_min = min;
    tm.tm_sec = sec;

    cfvariant res(cfvariant::DateTime);
    res.m_double = tmToDays(tm);
    auto *ret = new cfvariant(res);
    return ret;
}

} // namespace cfml
