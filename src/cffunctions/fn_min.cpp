/**
 * @file fn_min.cpp
 * @brief CFML min() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <climits>

namespace cfml {

cfvariant *cf_min(const cfvariant *arg1, const cfvariant *arg2) {
    if (!arg1 || !arg2) throw webstrada::exception("Min requires exactly 2 arguments");
    cfvariant *ret = nullptr;
    if (arg1->m_type == cfvariant::Number && arg2->m_type == cfvariant::Number) {
        ret = new cfvariant(std::min(arg1->m_int, arg2->m_int));
    } else {
        cfvariant res(cfvariant::Float);
        res.m_double = std::min(getDoubleValue(*arg1), getDoubleValue(*arg2));
        ret = new cfvariant(res);
    }
    return ret;
}

} // namespace cfml
