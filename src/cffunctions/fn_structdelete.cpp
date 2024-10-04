/**
 * @file fn_structdelete.cpp
 * @brief CFML structdelete() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <algorithm>
#include <set>
#include <vector>

namespace cfml {

cfvariant *cf_structdelete(cfvariant *str, const cfvariant *key, const cfvariant *indicateExisting) {
    if (!str || !key) throw webstrada::exception("StructDelete: Missing argument(s)");
    if (str->m_type != cfvariant::Struct) {
        throw webstrada::exception("StructDelete: First argument must be a structure");
    }
    string k = const_cast<cfvariant*>(key)->toString();
    k.toUpper();
    bool existed = str->m_struct->contains(k);
    if (existed) {
        struct_data_bump(str->m_structData);
        str->m_struct->erase(k);
    }
    bool ind = indicateExisting ? isTruthy(*indicateExisting) : false;
    auto *ret = new cfvariant(cfvariant::Boolean);
    ret->m_bool = ind ? existed : true;
    return ret;
}

} // namespace cfml
