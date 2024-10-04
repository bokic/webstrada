/**
 * @file fn_arraydeletenocase.cpp
 * @brief CFML arraydeletenocase() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <algorithm>
#include <vector>
#include <string>

namespace cfml {

cfvariant *cf_arraydeletenocase(cfvariant *arr, const cfvariant *val) {
    if (!arr || !val) throw webstrada::exception("ArrayDeleteNoCase: Missing argument(s)");
    if (arr->m_type != cfvariant::Array) {
        throw webstrada::exception("ArrayDeleteNoCase: First argument must be an array");
    }
    bool deleted = false;
    auto &vec = *arr->m_array;
    for (auto it = vec.begin(); it != vec.end(); ++it) {
        if (cfvariantsEqualNoCase(*it, *val)) {
            vec.erase(it);
            deleted = true;
            break;
        }
    }
    auto *ret = new cfvariant(cfvariant::Boolean);
    ret->m_bool = deleted;
    return ret;
}

} // namespace cfml
