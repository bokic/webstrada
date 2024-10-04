/**
 * @file fn_structinsert.cpp
 * @brief CFML structinsert() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <algorithm>
#include <set>
#include <vector>

namespace cfml {

cfvariant *cf_structinsert(cfvariant *str, const cfvariant *key, const cfvariant *val, const cfvariant *allowOverwrite) {
    if (!str || !key || !val) throw webstrada::exception("StructInsert: Missing argument(s)");
    if (str->m_type != cfvariant::Struct) {
        throw webstrada::exception("StructInsert: First argument must be a structure");
    }
    string k = const_cast<cfvariant*>(key)->toString();
    bool overwrite = allowOverwrite ? isTruthy(*allowOverwrite) : false;
    if (!overwrite && str->m_struct->contains(k)) {
        throw webstrada::exception("StructInsert: Key '" + k + "' already exists");
    }
    str->structSet(k, *val);
    auto *ret = new cfvariant(cfvariant::Boolean);
    ret->m_bool = true;
    return ret;
}

} // namespace cfml
