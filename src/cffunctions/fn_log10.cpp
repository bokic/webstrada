/**
 * @file fn_log10.cpp
 * @brief CFML log10() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <climits>

namespace cfml {

cfvariant *cf_log10(const cfvariant *arg) {
    if (!arg) throw webstrada::exception("Log10 requires exactly 1 argument");
    cfvariant res(cfvariant::Float);
    res.m_double = std::log10(getDoubleValue(*arg));
    auto *ret = new cfvariant(res);
    return ret;
}

} // namespace cfml
