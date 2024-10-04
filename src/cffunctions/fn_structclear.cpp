/**
 * @file fn_structclear.cpp
 * @brief CFML structclear() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <algorithm>
#include <set>
#include <vector>

namespace cfml {

cfvariant *cf_structclear(cfvariant *str) {
    if (!str) throw webstrada::exception("StructClear: Missing argument");
    if (str->m_type != cfvariant::Struct) {
        throw webstrada::exception("StructClear: Argument must be a structure");
    }
    struct_data_bump(str->m_structData);
    str->m_struct->clear();
    auto *ret = new cfvariant(cfvariant::Boolean);
    ret->m_bool = true;
    return ret;
}

} // namespace cfml
