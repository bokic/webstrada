/**
 * @file fn_structfind.cpp
 * @brief CFML structfind() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <algorithm>
#include <set>
#include <vector>

namespace cfml {

cfvariant *cf_structfind(const cfvariant *str, const cfvariant *key) {
    if (!str || !key) throw webstrada::exception("StructFind: Missing argument(s)");
    if (str->m_type != cfvariant::Struct) {
        throw webstrada::exception("StructFind: First argument must be a structure");
    }
    string k = const_cast<cfvariant*>(key)->toString();
    k.toUpper();
    auto it = str->m_struct->find(k);
    if (it == str->m_struct->end()) {
        throw webstrada::exception("StructFind: Key '" + k + "' not found");
    }
    auto *ret = new cfvariant(it->second);
    return ret;
}

} // namespace cfml
