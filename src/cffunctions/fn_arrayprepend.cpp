/**
 * @file fn_arrayprepend.cpp
 * @brief CFML arrayprepend() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <algorithm>
#include <vector>
#include <string>

namespace cfml {

cfvariant *cf_arrayprepend(cfvariant *arr, const cfvariant *val) {
    if (!arr || !val) throw webstrada::exception("ArrayPrepend: Missing argument(s)");
    if (arr->m_type != cfvariant::Array) {
        throw webstrada::exception("ArrayPrepend: First argument must be an array");
    }
    arr->m_array->insert(arr->m_array->begin(), *val);
    auto *ret = new cfvariant(cfvariant::Boolean);
    ret->m_bool = true;
    return ret;
}

} // namespace cfml
