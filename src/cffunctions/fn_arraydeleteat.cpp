/**
 * @file fn_arraydeleteat.cpp
 * @brief CFML arraydeleteat() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <algorithm>
#include <vector>
#include <string>

namespace cfml {

cfvariant *cf_arraydeleteat(cfvariant *arr, const cfvariant *idx) {
    if (!arr || !idx) throw webstrada::exception("ArrayDeleteAt: Missing argument(s)");
    if (arr->m_type != cfvariant::Array) {
        throw webstrada::exception("ArrayDeleteAt: First argument must be an array");
    }
    int index = getIntValue(*idx);
    if (index < 1 || index > (int)arr->m_array->size()) {
        throw webstrada::exception("ArrayDeleteAt: Index out of bounds");
    }
    arr->m_array->erase(arr->m_array->begin() + (index - 1));
    auto *ret = new cfvariant(cfvariant::Boolean);
    ret->m_bool = true;
    return ret;
}

} // namespace cfml
