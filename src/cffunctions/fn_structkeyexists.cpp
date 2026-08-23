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
    if (str->m_type != cfvariant::Struct && str->m_type != cfvariant::Component && str->m_type != cfvariant::Xml) {
        ret->m_bool = false;
    } else {
        string k = const_cast<cfvariant*>(key)->toString();
        ret->m_bool = str->m_struct && str->m_struct->contains(k);
        if (!ret->m_bool && str->m_type == cfvariant::Component) {
            string ku = k;
            ku.toUpper();
            ret->m_bool = cf_component_has_method(str, ku.constData()) != 0;
        } else if (!ret->m_bool && str->m_type == cfvariant::Xml && str->m_struct) {
            auto itChildren = str->m_struct->find("XMLCHILDREN");
            if (itChildren != str->m_struct->end() && itChildren->second.m_type == cfvariant::Array && itChildren->second.m_array) {
                for (auto const &child : *itChildren->second.m_array) {
                    if (child.m_type == cfvariant::Xml && child.m_struct) {
                        auto itName = child.m_struct->find("XMLNAME");
                        if (itName != child.m_struct->end() && itName->second.toString().compareCaseInsensitive(k) == 0) {
                            ret->m_bool = true;
                            break;
                        }
                    }
                }
            }
        }
    }
    return ret;
}

} // namespace cfml
