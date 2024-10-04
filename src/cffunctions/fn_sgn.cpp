/**
 * @file fn_sgn.cpp
 * @brief CFML sgn() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <climits>

namespace cfml {

cfvariant *cf_sgn(const cfvariant *arg) {
    if (!arg) throw webstrada::exception("Sgn requires exactly 1 argument");
    double val = getDoubleValue(*arg);
    cfvariant *ret = nullptr;
    if (val > 0.0) ret = new cfvariant(1);
    else if (val < 0.0) ret = new cfvariant(-1);
    else ret = new cfvariant(0);
    return ret;
}

} // namespace cfml
