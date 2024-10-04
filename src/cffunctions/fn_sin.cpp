/**
 * @file fn_sin.cpp
 * @brief CFML sin() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <climits>

namespace cfml {

cfvariant *cf_sin(const cfvariant *arg) {
    if (!arg) throw webstrada::exception("Sin requires exactly 1 argument");
    cfvariant res(cfvariant::Float);
    res.m_double = std::sin(getDoubleValue(*arg));
    auto *ret = new cfvariant(res);
    return ret;
}

} // namespace cfml
