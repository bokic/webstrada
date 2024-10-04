/**
 * @file fn_arrayresize.cpp
 * @brief CFML arrayresize() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <algorithm>
#include <vector>
#include <string>

namespace cfml {

cfvariant *cf_arrayresize(cfvariant *arr, const cfvariant *sz) {
    if (!arr || !sz) throw webstrada::exception("ArrayResize: Missing argument(s)");
    if (arr->m_type != cfvariant::Array) {
        throw webstrada::exception("ArrayResize: First argument must be an array");
    }
    int size = getIntValue(*sz);
    arr->m_array->resize(size);
    auto *ret = new cfvariant(cfvariant::Boolean);
    ret->m_bool = true;
    return ret;
}

} // namespace cfml
