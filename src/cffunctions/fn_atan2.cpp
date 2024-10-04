/**
 * @file fn_atan2.cpp
 * @brief CFML atan2() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <climits>

namespace cfml {

cfvariant *cf_atan2(const cfvariant *y, const cfvariant *x) {
    if (!y || !x) throw webstrada::exception("Atan2 requires exactly 2 arguments");
    cfvariant res(cfvariant::Float);
    res.m_double = std::atan2(getDoubleValue(*y), getDoubleValue(*x));
    auto *ret = new cfvariant(res);
    return ret;
}

} // namespace cfml
