/**
 * @file fn_arrayclear.cpp
 * @brief CFML arrayclear() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <algorithm>
#include <vector>
#include <string>

namespace cfml {

cfvariant *cf_arrayclear(cfvariant *arr) {
    if (!arr) throw webstrada::exception("ArrayClear: Missing argument");
    if (arr->m_type != cfvariant::Array) {
        throw webstrada::exception("ArrayClear: Argument must be an array");
    }
    arr->m_array->clear();
    auto *ret = new cfvariant(cfvariant::Boolean);
    ret->m_bool = true;
    return ret;
}

} // namespace cfml
