/**
 * @file fn_arrayinsertat.cpp
 * @brief CFML arrayinsertat() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <algorithm>
#include <vector>
#include <string>

namespace cfml {

cfvariant *cf_arrayinsertat(cfvariant *arr, const cfvariant *idx, const cfvariant *val) {
    if (!arr || !idx || !val) throw webstrada::exception("ArrayInsertAt: Missing argument(s)");
    if (arr->m_type != cfvariant::Array) {
        throw webstrada::exception("ArrayInsertAt: First argument must be an array");
    }
    int index = getIntValue(*idx);
    if (index < 1 || index > (int)arr->m_array->size() + 1) {
        throw webstrada::exception("ArrayInsertAt: Index out of bounds");
    }
    arr->m_array->insert(arr->m_array->begin() + (index - 1), *val);
    auto *ret = new cfvariant(cfvariant::Boolean);
    ret->m_bool = true;
    return ret;
}

} // namespace cfml
