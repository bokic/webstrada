/**
 * @file fn_arrayappend.cpp
 * @brief CFML arrayappend() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <algorithm>
#include <vector>
#include <string>

namespace cfml {

cfvariant *cf_arrayappend(cfvariant *arr, const cfvariant *val, const cfvariant *merge) {
    if (!arr || !val) throw webstrada::exception("ArrayAppend: Missing argument(s)");
    if (arr->m_isXmlNodeList) throwXmlNodeListUnsupported("ArrayAppend");
    if (!isCfArray(arr)) {
        throwNotArrayError(arr);
    }
    if (merge && isTruthy(*merge) && isCfArray(val)) {
        std::vector<cfvariant> values = *val->m_array;
        for (const auto &item : values) arr->insert(item);
    } else {
        arr->insert(*val);
    }
    auto *ret = new cfvariant(cfvariant::Boolean);
    ret->m_bool = true;
    return ret;
}

} // namespace cfml
