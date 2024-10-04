/**
 * @file fn_len.cpp
 * @brief CFML len() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <algorithm>
#include <cctype>
#include <climits>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

namespace cfml {

cfvariant *cf_len(const cfvariant *arg) {
    if (!arg) throw webstrada::exception("Len requires exactly 1 argument");
    int len = 0;
    if (arg->m_type == cfvariant::Binary && arg->m_binary) {
        len = static_cast<int>(arg->m_binary->size());
    } else if (isQueryColumnRef(arg)) {
        // A column reference behaves as its first cell for scalar operations
        // (CF 2021: Len(x) for x = q["a"] is the first cell's length, i.e. 1).
        cfvariant first = queryColumnFirstCell(arg);
        len = first.toString().length();
    } else {
        len = const_cast<cfvariant*>(arg)->toString().length();
    }
    auto *ret = new cfvariant(len);
    return ret;
}

} // namespace cfml
