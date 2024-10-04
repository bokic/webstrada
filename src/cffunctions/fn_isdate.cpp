/**
 * @file fn_isdate.cpp
 * @brief CFML isdate() built-in.
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

cfvariant *cf_isdate(const cfvariant *arg) {
    if (!arg) throw webstrada::exception("IsDate requires exactly 1 argument");
    bool is_d = false;
    if (arg->m_type == cfvariant::DateTime) {
        is_d = true;
    } else {
        double dummy = 0.0;
        is_d = parseDateTimeStr(const_cast<cfvariant*>(arg)->toString(), dummy);
    }
    cfvariant res(cfvariant::Boolean);
    res.m_bool = is_d;
    auto *ret = new cfvariant(res);
    return ret;
}

} // namespace cfml
