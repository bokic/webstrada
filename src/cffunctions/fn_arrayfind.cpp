/**
 * @file fn_arrayfind.cpp
 * @brief CFML arrayfind() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <algorithm>
#include <vector>
#include <string>

namespace cfml {

cfvariant *cf_arrayfind(const cfvariant *arr, const cfvariant *val) {
    if (!arr || !val) throw webstrada::exception("ArrayFind: Missing argument(s)");
    if (arr->m_type != cfvariant::Array) {
        throw webstrada::exception("ArrayFind: First argument must be an array");
    }
    int foundIdx = 0;
    for (size_t i = 0; i < arr->m_array->size(); i++) {
        if (cfvariantsEqual(arr->m_array->at(i), *val)) {
            foundIdx = static_cast<int>(i + 1);
            break;
        }
    }
    auto *ret = new cfvariant(foundIdx);
    return ret;
}

} // namespace cfml
