/**
 * @file fn_isleapyear.cpp
 * @brief CFML isleapyear() built-in.
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

cfvariant *cf_isleapyear(const cfvariant *year) {
    if (!year) throw webstrada::exception("IsLeapYear requires exactly 1 argument");
    int yr = getIntValue(*year);
    cfvariant res(cfvariant::Boolean);
    res.m_bool = isLeapYear(yr);
    auto *ret = new cfvariant(res);
    return ret;
}

} // namespace cfml
