/**
 * @file fn_arraylast.cpp
 * @brief CFML arraylast() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>

namespace cfml {

cfvariant *cf_arraylast(const cfvariant *arr) {
    if (!arr) throw webstrada::exception("ArrayLast: Missing argument");
    if (arr->m_type != cfvariant::Array || !isCfArray(arr)) {
        throwNotArrayError(arr);
    }
    if (arr->m_array->empty()) {
        // CF reports type "java.lang.RuntimeException" for this error (verified
        // on CF 2025).
        throw webstrada::exception("java.lang.RuntimeException",
            "Array is empty.Cannot return last element of array.", "");
    }
    auto *ret = new cfvariant(arr->m_array->back());
    return ret;
}

} // namespace cfml
