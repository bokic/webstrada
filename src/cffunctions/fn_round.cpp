/**
 * @file fn_round.cpp
 * @brief CFML round() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <climits>

namespace cfml {

cfvariant *cf_round(const cfvariant *arg) {
    if (!arg) throw webstrada::exception("Round requires exactly 1 argument");
    auto *ret = new cfvariant(static_cast<int>(std::round(getDoubleValue(*arg))));
    return ret;
}

} // namespace cfml
