/**
 * @file fn_arraycontains.cpp
 * @brief CFML arraycontains() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <algorithm>
#include <vector>
#include <string>

namespace cfml {

cfvariant *cf_arraycontains(const cfvariant *arr, const cfvariant *val) {
    if (!arr || !val) throw webstrada::exception("ArrayContains: Missing argument(s)");
    if (arr->m_type != cfvariant::Array) {
        throw webstrada::exception("ArrayContains: First argument must be an array");
    }
    bool found = false;
    for (const auto &item : *arr->m_array) {
        if (cfvariantsEqual(item, *val)) {
            found = true;
            break;
        }
    }
    auto *ret = new cfvariant(cfvariant::Boolean);
    ret->m_bool = found;
    return ret;
}

} // namespace cfml
