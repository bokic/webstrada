/**
 * @file fn_exp.cpp
 * @brief CFML exp() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <climits>

namespace cfml {

cfvariant *cf_exp(const cfvariant *arg) {
    if (!arg) throw webstrada::exception("Exp requires exactly 1 argument");
    cfvariant res(cfvariant::Float);
    res.m_double = std::exp(getDoubleValue(*arg));
    auto *ret = new cfvariant(res);
    return ret;
}

} // namespace cfml
