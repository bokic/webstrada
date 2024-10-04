/**
 * @file fn_structisempty.cpp
 * @brief CFML structisempty() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <algorithm>
#include <set>
#include <vector>

namespace cfml {

cfvariant *cf_structisempty(const cfvariant *str) {
    if (!str) throw webstrada::exception("StructIsEmpty: Missing argument");
    if (str->m_type != cfvariant::Struct && str->m_type != cfvariant::Component) {
        throw webstrada::exception("StructIsEmpty: Argument must be a structure");
    }
    auto *ret = new cfvariant(cfvariant::Boolean);
    ret->m_bool = str->m_struct->empty();
    if (str->m_type == cfvariant::Component) {
        std::vector<webstrada::string> methodKeys;
        cf_component_append_method_keys(str, methodKeys);
        if (!methodKeys.empty()) ret->m_bool = false;
    }
    return ret;
}

} // namespace cfml
