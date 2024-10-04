/**
 * @file fn_arrayisdefined.cpp
 * @brief CFML arrayisdefined() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <algorithm>
#include <vector>
#include <string>

namespace cfml {

cfvariant *cf_arrayisdefined(const cfvariant *arr, const cfvariant *idx) {
    if (!arr || !idx) throw webstrada::exception("ArrayIsDefined: Missing argument(s)");
    if (arr->m_type != cfvariant::Array) {
        throw webstrada::exception("ArrayIsDefined: First argument must be an array");
    }
    int index = getIntValue(*idx);
    auto *ret = new cfvariant(cfvariant::Boolean);
    ret->m_bool = (index >= 1 && index <= (int)arr->m_array->size());
    return ret;
}

} // namespace cfml
