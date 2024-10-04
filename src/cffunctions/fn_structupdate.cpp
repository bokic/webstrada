/**
 * @file fn_structupdate.cpp
 * @brief CFML structupdate() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <algorithm>
#include <set>
#include <vector>

namespace cfml {

cfvariant *cf_structupdate(cfvariant *str, const cfvariant *key, const cfvariant *val) {
    if (!str || !key || !val) throw webstrada::exception("StructUpdate: Missing argument(s)");
    if (str->m_type != cfvariant::Struct) {
        throw webstrada::exception("StructUpdate: First argument must be a structure");
    }
    string k = const_cast<cfvariant*>(key)->toString();
    if (!str->m_struct->contains(k)) {
        throw webstrada::exception("StructUpdate: Key '" + k + "' does not exist");
    }
    str->structSet(k, *val);
    auto *ret = new cfvariant(cfvariant::Boolean);
    ret->m_bool = true;
    return ret;
}

} // namespace cfml
