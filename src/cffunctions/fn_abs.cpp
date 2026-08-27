/**
 * @file fn_abs.cpp
 * @brief CFML abs() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <climits>

namespace cfml {

cfvariant *cf_abs(const cfvariant *arg) {
    if (!arg) throw webstrada::exception("Abs requires exactly 1 argument");
    cfvariant *ret = nullptr;
    if (arg->m_type == cfvariant::Number) {
        ret = new cfvariant(std::abs(arg->m_int));
    } else if (arg->m_type == cfvariant::Long) {
        __int128 res = arg->m_long < 0 ? -static_cast<__int128>(arg->m_long) : static_cast<__int128>(arg->m_long);
        if (res >= -2147483648LL && res <= 2147483647LL) {
            ret = new cfvariant(static_cast<int>(res));
        } else if (res <= (__int128)LLONG_MAX) {
            ret = new cfvariant(cfvariant::Long);
            ret->m_long = static_cast<long long>(res);
        } else {
            cfvariant r2(cfvariant::Float);
            r2.m_double = static_cast<double>(res);
            ret = new cfvariant(r2);
        }
    } else {
        cfvariant res(cfvariant::Float);
        res.m_double = std::abs(getDoubleValue(*arg));
        ret = new cfvariant(res);
    }
    return ret;
}

} // namespace cfml
