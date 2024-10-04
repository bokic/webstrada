/**
 * @file fn_structappend.cpp
 * @brief CFML structappend() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <algorithm>
#include <set>
#include <vector>

namespace cfml {

cfvariant *cf_structappend(cfvariant *dest, const cfvariant *source, const cfvariant *overwriteFlag) {
    if (!dest || dest->m_type != cfvariant::Struct) {
        throw webstrada::exception("StructAppend: First argument must be a struct");
    }
    if (!source || source->m_type != cfvariant::Struct) {
        throw webstrada::exception("StructAppend: Second argument must be a struct");
    }
    bool overwrite = true;
    if (overwriteFlag && overwriteFlag->m_type != cfvariant::Null) overwrite = isTruthy(*overwriteFlag);
    for (const auto &key : structOrderedKeys(*source)) {
        const cfvariant &val = source->m_struct->at(key);
        if (overwrite || !dest->m_struct->contains(key)) {
            dest->structSet(key, val);
        }
    }
    auto *ret = new cfvariant(cfvariant::Boolean);
    ret->m_bool = true;
    return ret;
}

} // namespace cfml
