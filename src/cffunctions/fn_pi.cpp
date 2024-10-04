/**
 * @file fn_pi.cpp
 * @brief CFML pi() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <climits>

namespace cfml {

cfvariant *cf_pi() {
    cfvariant res(cfvariant::Float);
    res.m_double = 3.14159265358979323846;
    auto *ret = new cfvariant(res);
    return ret;
}

} // namespace cfml
