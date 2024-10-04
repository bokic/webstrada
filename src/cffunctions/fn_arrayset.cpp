/**
 * @file fn_arrayset.cpp
 * @brief CFML arrayset() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <algorithm>
#include <vector>
#include <string>

namespace cfml {

cfvariant *cf_arrayset(cfvariant *arr, const cfvariant *start, const cfvariant *end, const cfvariant *val) {
    if (!arr || !start || !end || !val) throw webstrada::exception("ArraySet: Missing argument(s)");
    if (arr->m_type != cfvariant::Array) {
        throw webstrada::exception("ArraySet: First argument must be an array");
    }
    int s = getIntValue(*start);
    int e = getIntValue(*end);
    for (int i = s; i <= e; i++) {
        if (i >= 1 && i <= (int)arr->m_array->size()) {
            arr->m_array->at(i - 1) = *val;
        }
    }
    auto *ret = new cfvariant(cfvariant::Boolean);
    ret->m_bool = true;
    return ret;
}

} // namespace cfml
