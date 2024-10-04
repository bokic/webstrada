/**
 * @file fn_acos.cpp
 * @brief CFML acos() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <climits>

namespace cfml {

cfvariant *cf_acos(const cfvariant *arg) {
    if (!arg) throw webstrada::exception("Acos requires exactly 1 argument");
    cfvariant res(cfvariant::Float);
    res.m_double = std::acos(getDoubleValue(*arg));
    auto *ret = new cfvariant(res);
    return ret;
}

} // namespace cfml
