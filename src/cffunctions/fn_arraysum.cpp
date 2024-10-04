/**
 * @file fn_arraysum.cpp
 * @brief CFML arraysum() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <algorithm>
#include <vector>
#include <string>

namespace cfml {

cfvariant *cf_arraysum(const cfvariant *arr) {
    if (!arr) throw webstrada::exception("ArraySum: Missing argument");
    if (arr->m_type != cfvariant::Array) {
        throw webstrada::exception("ArraySum: Argument is not an array");
    }
    double sum = 0;
    for (const auto &val : *arr->m_array) {
        sum += getDoubleValue(val);
    }
    auto *ret = new cfvariant(cfvariant::Float);
    ret->m_double = sum;
    return ret;
}

} // namespace cfml
