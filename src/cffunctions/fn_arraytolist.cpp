/**
 * @file fn_arraytolist.cpp
 * @brief CFML arraytolist() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <algorithm>
#include <vector>
#include <string>

namespace cfml {

cfvariant *cf_arraytolist(const cfvariant *arr, const cfvariant *delim) {
    if (!arr) throw webstrada::exception("ArrayToList: Missing argument");
    if (arr->m_type != cfvariant::Array) {
        throw webstrada::exception("ArrayToList: First argument must be an array");
    }
    string d = ",";
    if (delim) d = const_cast<cfvariant*>(delim)->toString();
    string listStr;
    for (size_t k = 0; k < arr->m_array->size(); k++) {
        if (k > 0) listStr += d;
        listStr += variantToString(arr->m_array->at(k));
    }
    auto *ret = new cfvariant(listStr);
    return ret;
}

} // namespace cfml
