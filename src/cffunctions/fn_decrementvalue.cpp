/**
 * @file fn_decrementvalue.cpp
 * @brief CFML decrementvalue() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <climits>

namespace cfml {

cfvariant *cf_decrementvalue(const cfvariant *arg) {
    if (!arg) throw webstrada::exception("DecrementValue requires exactly 1 argument");
    cfvariant *ret = nullptr;
    if (arg->m_type == cfvariant::Number) {
        ret = new cfvariant(arg->m_int - 1);
    } else if (arg->m_type == cfvariant::Long) {
        ret = new cfvariant(cfvariant::Long);
        ret->m_long = arg->m_long - 1;
    } else {
        cfvariant res(cfvariant::Float);
        res.m_double = getDoubleValue(*arg) - 1.0;
        ret = new cfvariant(res);
    }
    return ret;
}

} // namespace cfml
