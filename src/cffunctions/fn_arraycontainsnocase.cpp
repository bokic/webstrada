/**
 * @file fn_arraycontainsnocase.cpp
 * @brief CFML arraycontainsnocase() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <algorithm>
#include <vector>
#include <string>

namespace cfml {

cfvariant *cf_arraycontainsnocase(const cfvariant *arr, const cfvariant *val) {
    if (!arr || !val) throw webstrada::exception("ArrayContainsNoCase: Missing argument(s)");
    if (arr->m_type != cfvariant::Array) {
        throw webstrada::exception("ArrayContainsNoCase: First argument must be an array");
    }
    bool found = false;
    for (const auto &item : *arr->m_array) {
        if (cfvariantsEqualNoCase(item, *val)) {
            found = true;
            break;
        }
    }
    auto *ret = new cfvariant(cfvariant::Boolean);
    ret->m_bool = found;
    return ret;
}

} // namespace cfml
