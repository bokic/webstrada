/**
 * @file fn_isquery.cpp
 * @brief CFML isquery() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <string>
#include <vector>

namespace cfml {

cfvariant *cf_isquery(const cfvariant *val) {
    auto *ret = new cfvariant(cfvariant::Boolean);
    ret->m_bool = val && (val->m_type == cfvariant::Query);
    return ret;
}

} // namespace cfml
