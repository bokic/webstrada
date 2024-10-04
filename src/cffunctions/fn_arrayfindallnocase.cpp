/**
 * @file fn_arrayfindallnocase.cpp
 * @brief CFML arrayfindallnocase() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <algorithm>
#include <vector>
#include <string>

namespace cfml {

cfvariant *cf_arrayfindallnocase(const cfvariant *arr, const cfvariant *val) {
    if (!arr || !val) throw webstrada::exception("ArrayFindAllNoCase: Missing argument(s)");
    if (arr->m_type != cfvariant::Array) {
        throw webstrada::exception("ArrayFindAllNoCase: First argument must be an array");
    }
    auto *ret = new cfvariant(cfvariant::Array);
    for (size_t i = 0; i < arr->m_array->size(); i++) {
        if (cfvariantsEqualNoCase(arr->m_array->at(i), *val)) {
            ret->insert(cfvariant(static_cast<int>(i + 1)));
        }
    }
    return ret;
}

} // namespace cfml
