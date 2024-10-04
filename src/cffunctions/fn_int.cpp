/**
 * @file fn_int.cpp
 * @brief CFML int() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <climits>

namespace cfml {

cfvariant *cf_int(const cfvariant *arg) {
    if (!arg) throw webstrada::exception("Int requires exactly 1 argument");
    auto *ret = new cfvariant(static_cast<int>(std::floor(getDoubleValue(*arg))));
    return ret;
}

} // namespace cfml
