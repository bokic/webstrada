/**
 * @file fn_createdate.cpp
 * @brief CFML createdate() built-in.
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

cfvariant *cf_createdate(const cfvariant *yrVal, const cfvariant *monVal, const cfvariant *dayVal) {
    if (!yrVal || !monVal || !dayVal) throw webstrada::exception("CreateDate requires exactly 3 arguments");
    int yr = getIntValue(*yrVal);
    int mon = getIntValue(*monVal);
    int day = getIntValue(*dayVal);

    struct tm tm;
    memset(&tm, 0, sizeof(tm));
    tm.tm_year = yr - 1900;
    tm.tm_mon = mon - 1;
    tm.tm_mday = day;

    cfvariant res(cfvariant::DateTime);
    res.m_double = tmToDays(tm);
    auto *ret = new cfvariant(res);
    return ret;
}

} // namespace cfml
