/**
 * @file fn_atan.cpp
 * @brief CFML atan() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <climits>

namespace cfml {

cfvariant *cf_atan(const cfvariant *arg) {
    if (!arg) throw webstrada::exception("Atan requires exactly 1 argument");
    cfvariant res(cfvariant::Float);
    res.m_double = std::atan(getDoubleValue(*arg));
    auto *ret = new cfvariant(res);
    return ret;
}

} // namespace cfml
