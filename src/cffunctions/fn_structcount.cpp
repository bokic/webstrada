/**
 * @file fn_structcount.cpp
 * @brief CFML structcount() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <algorithm>
#include <set>
#include <vector>

namespace cfml {

cfvariant *cf_structcount(const cfvariant *str) {
    if (!str) throw webstrada::exception("StructCount: Missing argument");
    if (str->m_type != cfvariant::Struct && str->m_type != cfvariant::Component) {
        throw webstrada::exception("StructCount: Argument must be a structure");
    }
    int count = str->m_isArguments ? static_cast<int>(argumentsVisibleKeys(str).size())
                                   : static_cast<int>(str->m_struct->size());
    if (str->m_type == cfvariant::Component) {
        std::vector<webstrada::string> methodKeys;
        cf_component_append_method_keys(str, methodKeys);
        count += static_cast<int>(methodKeys.size());
    }
    auto *ret = new cfvariant(count);
    return ret;
}

} // namespace cfml
