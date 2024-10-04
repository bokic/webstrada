/**
 * @file fn_structvaluearray.cpp
 * @brief CFML structvaluearray() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <algorithm>
#include <vector>
#include <string>

namespace cfml {

cfvariant *cf_structvaluearray(const cfvariant *str) {
    if (!str) throw webstrada::exception("StructValueArray: Missing argument");
    if (str->m_type != cfvariant::Struct) {
        throw webstrada::exception("StructValueArray: Argument is not a structure");
    }
    auto *ret = new cfvariant(cfvariant::Array);
    for (auto &pair : *str->m_struct) {
        ret->insert(pair.second);
    }
    return ret;
}

} // namespace cfml
