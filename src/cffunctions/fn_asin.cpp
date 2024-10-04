/**
 * @file fn_asin.cpp
 * @brief CFML asin() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <climits>

namespace cfml {

cfvariant *cf_asin(const cfvariant *arg) {
    if (!arg) throw webstrada::exception("Asin requires exactly 1 argument");
    cfvariant res(cfvariant::Float);
    res.m_double = std::asin(getDoubleValue(*arg));
    auto *ret = new cfvariant(res);
    return ret;
}

} // namespace cfml
