/**
 * @file fn_isnumericdate.cpp
 * @brief CFML isnumericdate() built-in.
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

cfvariant *cf_isnumericdate(const cfvariant *value) {
    if (!value) throw webstrada::exception("IsNumericDate requires exactly 1 argument");
    cfvariant res(cfvariant::Boolean);
    res.m_bool = (value->m_type == cfvariant::Number || value->m_type == cfvariant::Float || value->m_type == cfvariant::DateTime);
    auto *ret = new cfvariant(res);
    return ret;
}

} // namespace cfml
