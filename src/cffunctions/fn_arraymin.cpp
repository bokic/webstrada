/**
 * @file fn_arraymin.cpp
 * @brief CFML arraymin() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <algorithm>
#include <vector>
#include <string>

namespace cfml {

cfvariant *cf_arraymin(const cfvariant *arr) {
    if (!arr) throw webstrada::exception("ArrayMin: Missing argument");
    if (arr->m_type != cfvariant::Array) {
        throw webstrada::exception("ArrayMin: Argument is not an array");
    }
    if (arr->m_array->empty()) {
        auto *ret = new cfvariant(cfvariant::Float);
        ret->m_double = 0.0;
        return ret;
    }
    double minVal = getDoubleValue(arr->m_array->front());
    for (const auto &val : *arr->m_array) {
        double d = getDoubleValue(val);
        if (d < minVal) minVal = d;
    }
    auto *ret = new cfvariant(cfvariant::Float);
    ret->m_double = minVal;
    return ret;
}

} // namespace cfml
