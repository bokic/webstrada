/**
 * @file fn_arraylen.cpp
 * @brief CFML arraylen() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <algorithm>
#include <vector>
#include <string>

namespace cfml {

cfvariant *cf_arraylen(const cfvariant *arr) {
    if (!arr) throw webstrada::exception("ArrayLen: Missing argument");
    if (arr->m_type == cfvariant::Struct && arr->m_isArguments) {
        // CF supports ArrayLen(arguments) even though arguments is a struct.
        auto *ret = new cfvariant(static_cast<int>(argumentsVisibleKeys(arr).size()));
        return ret;
    }
    // An XmlNodeArray (multi same-name child group) is a Java List: ArrayLen
    // works even though IsArray() is NO (verified on the RDS host).
    if (arr->m_type != cfvariant::Array || (!isCfArray(arr) && !arr->m_isXmlNodeList)) {
        throwNotArrayError(arr);
    }
    auto *ret = new cfvariant(static_cast<int>(arr->m_array->size()));
    return ret;
}

} // namespace cfml
