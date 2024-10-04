/**
 * @file fn_structkeyexists.cpp
 * @brief CFML structkeyexists() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <algorithm>
#include <set>
#include <vector>

namespace cfml {

cfvariant *cf_structkeyexists(const cfvariant *str, const cfvariant *key) {
    if (!str || !key) throw webstrada::exception("StructKeyExists: Missing argument(s)");
    auto *ret = new cfvariant(cfvariant::Boolean);
    if (str->m_type != cfvariant::Struct && str->m_type != cfvariant::Component) {
        ret->m_bool = false;
    } else {
        string k = const_cast<cfvariant*>(key)->toString();
        k.toUpper();
        ret->m_bool = str->m_struct->contains(k);
        if (!ret->m_bool && str->m_type == cfvariant::Component) {
            ret->m_bool = cf_component_has_method(str, k.constData()) != 0;
        }
    }
    return ret;
}

} // namespace cfml
