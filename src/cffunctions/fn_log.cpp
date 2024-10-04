/**
 * @file fn_log.cpp
 * @brief CFML log() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <climits>

namespace cfml {

cfvariant *cf_log(const cfvariant *arg) {
    if (!arg) throw webstrada::exception("Log requires exactly 1 argument");
    cfvariant res(cfvariant::Float);
    res.m_double = std::log(getDoubleValue(*arg));
    auto *ret = new cfvariant(res);
    return ret;
}

} // namespace cfml
