/**
 * @file fn_parsedatetime.cpp
 * @brief CFML parsedatetime() built-in.
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

cfvariant *cf_parsedatetime(const cfvariant *date_string, const cfvariant *popup) {
    if (!date_string) throw webstrada::exception("ParseDateTime requires at least 1 argument");
    double days = 0.0;
    if (!parseDateTimeStr(const_cast<cfvariant*>(date_string)->toString(), days)) {
        throw webstrada::exception("ParseDateTime: Invalid date/time value");
    }
    cfvariant res(cfvariant::DateTime);
    res.m_double = days;
    auto *ret = new cfvariant(res);
    return ret;
}

} // namespace cfml
