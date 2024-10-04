/**
 * @file fn_createdatetime.cpp
 * @brief CFML createdatetime() built-in.
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

cfvariant *cf_createdatetime(const cfvariant *yrVal, const cfvariant *monVal, const cfvariant *dayVal, const cfvariant *hrVal, const cfvariant *minVal, const cfvariant *secVal) {
    if (!yrVal || !monVal || !dayVal || !hrVal || !minVal || !secVal) throw webstrada::exception("CreateDateTime requires exactly 6 arguments");
    int yr = getIntValue(*yrVal);
    int mon = getIntValue(*monVal);
    int day = getIntValue(*dayVal);
    int hr = getIntValue(*hrVal);
    int min = getIntValue(*minVal);
    int sec = getIntValue(*secVal);

    struct tm tm;
    memset(&tm, 0, sizeof(tm));
    tm.tm_year = yr - 1900;
    tm.tm_mon = mon - 1;
    tm.tm_mday = day;
    tm.tm_hour = hr;
    tm.tm_min = min;
    tm.tm_sec = sec;

    cfvariant res(cfvariant::DateTime);
    res.m_double = tmToDays(tm);
    auto *ret = new cfvariant(res);
    return ret;
}

} // namespace cfml
