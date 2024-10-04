/**
 * @file fn_arrayshift.cpp
 * @brief CFML arrayshift() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>

namespace cfml {

cfvariant *cf_arrayshift(cfvariant *arr) {
    if (!arr) throw webstrada::exception("ArrayShift: Missing argument");
    if (arr->m_isXmlNodeList) throwXmlNodeListUnsupported("ArrayShift");
    if (arr->m_type != cfvariant::Array || !isCfArray(arr)) {
        throwNotArrayError(arr);
    }
    if (arr->m_array->empty()) {
        throw webstrada::exception("Empty Array.");
    }
    cfvariant *ret = new cfvariant(arr->m_array->front());
    arr->m_array->erase(arr->m_array->begin());
    return ret;
}

} // namespace cfml
