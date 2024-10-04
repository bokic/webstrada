/**
 * @file fn_ceiling.cpp
 * @brief CFML ceiling() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <climits>

namespace cfml {

cfvariant *cf_ceiling(const cfvariant *arg) {
    if (!arg) throw webstrada::exception("Ceiling requires exactly 1 argument");
    auto *ret = new cfvariant(static_cast<int>(std::ceil(getDoubleValue(*arg))));
    return ret;
}

} // namespace cfml
