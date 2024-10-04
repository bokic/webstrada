/**
 * @file fn_arrayfirst.cpp
 * @brief CFML arrayfirst() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>

namespace cfml {

cfvariant *cf_arrayfirst(const cfvariant *arr) {
    if (!arr) throw webstrada::exception("ArrayFirst: Missing argument");
    if (arr->m_type != cfvariant::Array || !isCfArray(arr)) {
        throwNotArrayError(arr);
    }
    if (arr->m_array->empty()) {
        // CF reports type "java.lang.RuntimeException" for this error (verified
        // on CF 2025).
        throw webstrada::exception("java.lang.RuntimeException",
            "Array is empty.Cannot return first element of array.", "");
    }
    auto *ret = new cfvariant(arr->m_array->front());
    return ret;
}

} // namespace cfml
