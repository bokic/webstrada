/**
 * @file fn_arrayavg.cpp
 * @brief CFML arrayavg() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <algorithm>
#include <vector>
#include <string>

namespace cfml {

cfvariant *cf_arrayavg(const cfvariant *arr) {
    if (!arr) throw webstrada::exception("ArrayAvg: Missing argument");
    if (arr->m_type != cfvariant::Array) {
        throw webstrada::exception("ArrayAvg: Argument is not an array");
    }
    double sum = 0;
    int count = 0;
    for (const auto &val : *arr->m_array) {
        sum += getDoubleValue(val);
        count++;
    }
    auto *ret = new cfvariant(cfvariant::Float);
    ret->m_double = count > 0 ? (sum / count) : 0.0;
    return ret;
}

} // namespace cfml
