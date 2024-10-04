/**
 * @file fn_now.cpp
 * @brief CFML now() built-in.
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

cfvariant *cf_now() {
    std::time_t t = std::time(nullptr);
    struct tm tm_local;
    localtime_r(&t, &tm_local);
    cfvariant res(cfvariant::DateTime);
    res.m_double = tmToDays(tm_local);
    auto *ret = new cfvariant(res);
    return ret;
}

} // namespace cfml
