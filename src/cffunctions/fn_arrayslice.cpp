/**
 * @file fn_arrayslice.cpp
 * @brief CFML arrayslice() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <algorithm>
#include <vector>
#include <string>

namespace cfml {

cfvariant *cf_arrayslice(const cfvariant *arr, const cfvariant *offset, const cfvariant *length) {
    if (!arr || !offset) throw webstrada::exception("ArraySlice: Missing argument(s)");
    if (arr->m_type != cfvariant::Array) {
        throw webstrada::exception("ArraySlice: First argument must be an array");
    }
    int off = getIntValue(*offset);
    int len = length ? getIntValue(*length) : -1;
    if (off < 1) off = 1;
    auto *ret = new cfvariant(cfvariant::Array);
    if (off <= (int)arr->m_array->size()) {
        size_t start = off - 1;
        size_t count = (len < 0) ? (arr->m_array->size() - start) : static_cast<size_t>(len);
        for (size_t i = 0; i < count && (start + i) < arr->m_array->size(); i++) {
            ret->insert(arr->m_array->at(start + i));
        }
    }
    return ret;
}

} // namespace cfml
