/**
 * @file fn_arraypop.cpp
 * @brief CFML arraypop() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>

namespace cfml {

cfvariant *cf_arraypop(cfvariant *arr) {
    if (!arr) throw webstrada::exception("ArrayPop: Missing argument");
    if (arr->m_isXmlNodeList) throwXmlNodeListUnsupported("ArrayPop");
    if (arr->m_type != cfvariant::Array || !isCfArray(arr)) {
        throwNotArrayError(arr);
    }
    if (arr->m_array->empty()) {
        throw webstrada::exception("Empty Array.");
    }
    cfvariant *ret = new cfvariant(arr->m_array->back());
    arr->m_array->pop_back();
    return ret;
}

} // namespace cfml
