/**
 * @file fn_isarray.cpp
 * @brief CFML isarray() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <algorithm>
#include <vector>
#include <string>

namespace cfml {

cfvariant *cf_isarray(const cfvariant *val) {
    auto *ret = new cfvariant(cfvariant::Boolean);
    ret->m_bool = isCfArray(val);
    return ret;
}

} // namespace cfml
