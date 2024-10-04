/**
 * @file fn_arraymax.cpp
 * @brief CFML arraymax() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <algorithm>
#include <vector>
#include <string>

namespace cfml {

cfvariant *cf_arraymax(const cfvariant *arr) {
    if (!arr) throw webstrada::exception("ArrayMax: Missing argument");
    if (arr->m_type != cfvariant::Array) {
        throw webstrada::exception("ArrayMax: Argument is not an array");
    }
    if (arr->m_array->empty()) {
        auto *ret = new cfvariant(cfvariant::Float);
        ret->m_double = 0.0;
        return ret;
    }
    double maxVal = getDoubleValue(arr->m_array->front());
    for (const auto &val : *arr->m_array) {
        double d = getDoubleValue(val);
        if (d > maxVal) maxVal = d;
    }
    auto *ret = new cfvariant(cfvariant::Float);
    ret->m_double = maxVal;
    return ret;
}

} // namespace cfml
