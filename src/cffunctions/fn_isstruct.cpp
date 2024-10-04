/**
 * @file fn_isstruct.cpp
 * @brief CFML isstruct() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <algorithm>
#include <set>
#include <vector>

namespace cfml {

cfvariant *cf_isstruct(const cfvariant *val) {
    auto *ret = new cfvariant(cfvariant::Boolean);
    // ColdFusion reports a CFC instance as a struct too (verified on CF 2025:
    // IsStruct(CreateObject("component",...)) -> YES).
    ret->m_bool = val && (val->m_type == cfvariant::Struct || val->m_type == cfvariant::Component);
    return ret;
}

} // namespace cfml
