/**
 * @file fn_arrayswap.cpp
 * @brief CFML arrayswap() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <algorithm>
#include <vector>
#include <string>

namespace cfml {

cfvariant *cf_arrayswap(cfvariant *arr, const cfvariant *idx1, const cfvariant *idx2) {
    if (!arr || !idx1 || !idx2) throw webstrada::exception("ArraySwap: Missing argument(s)");
    if (arr->m_type != cfvariant::Array) {
        throw webstrada::exception("ArraySwap: First argument must be an array");
    }
    int i1 = getIntValue(*idx1);
    int i2 = getIntValue(*idx2);
    if (i1 < 1 || i1 > (int)arr->m_array->size() || i2 < 1 || i2 > (int)arr->m_array->size()) {
        throw webstrada::exception("ArraySwap: Index out of bounds");
    }
    std::swap(arr->m_array->at(i1 - 1), arr->m_array->at(i2 - 1));
    auto *ret = new cfvariant(cfvariant::Boolean);
    ret->m_bool = true;
    return ret;
}

} // namespace cfml
