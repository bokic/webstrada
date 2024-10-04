/**
 * @file fn_cos.cpp
 * @brief CFML cos() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <climits>

namespace cfml {

cfvariant *cf_cos(const cfvariant *arg) {
    if (!arg) throw webstrada::exception("Cos requires exactly 1 argument");
    cfvariant res(cfvariant::Float);
    res.m_double = std::cos(getDoubleValue(*arg));
    auto *ret = new cfvariant(res);
    return ret;
}

} // namespace cfml
