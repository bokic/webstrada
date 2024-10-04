/**
 * @file fn_hour.cpp
 * @brief CFML hour() built-in.
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

cfvariant *cf_hour(const cfvariant *arg) {
    if (!arg) throw webstrada::exception("Hour requires exactly 1 argument");
    double days = 0.0;
    if (arg->m_type == cfvariant::DateTime) {
        days = arg->m_double;
    } else if (arg->m_type == cfvariant::Number) {
        days = arg->m_int;
    } else if (arg->m_type == cfvariant::Long) {
        days = static_cast<double>(arg->m_long);
    } else if (arg->m_type == cfvariant::Float) {
        days = arg->m_double;
    } else {
        if (!parseDateTimeStr(const_cast<cfvariant*>(arg)->toString(), days)) {
            throw webstrada::exception("Hour: Invalid date/time value");
        }
    }
    struct tm tm = daysToTm(days);
    auto *ret = new cfvariant(tm.tm_hour);
    return ret;
}

} // namespace cfml
