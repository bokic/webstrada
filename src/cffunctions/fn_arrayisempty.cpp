/**
 * @file fn_arrayisempty.cpp
 * @brief CFML arrayisempty() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <algorithm>
#include <vector>
#include <string>

namespace cfml {

cfvariant *cf_arrayisempty(const cfvariant *arr) {
    if (!arr) throw webstrada::exception("ArrayIsEmpty: Missing argument");
    if (arr->m_type != cfvariant::Array) {
        throw webstrada::exception("ArrayIsEmpty: Argument is not an array");
    }
    auto *ret = new cfvariant(cfvariant::Boolean);
    ret->m_bool = arr->m_array->empty();
    return ret;
}

} // namespace cfml
