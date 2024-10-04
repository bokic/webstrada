/**
 * @file fn_find.cpp
 * @brief CFML find() built-in.
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

cfvariant *cf_find(const cfvariant *subVal, const cfvariant *strVal, const cfvariant *startVal) {
    if (!subVal || !strVal) throw webstrada::exception("Find requires at least 2 arguments");
    int start = 1;
    if (startVal) {
        start = (startVal->m_type == cfvariant::Number) ? startVal->m_int : atoi(const_cast<cfvariant*>(startVal)->toString().constData());
    }
    webstrada::string sub = const_cast<cfvariant*>(subVal)->toString();
    webstrada::string s = const_cast<cfvariant*>(strVal)->toString();
    if (start < 1 || start > s.length()) {
        auto *ret = new cfvariant(0);
        return ret;
    }
    int pos = s.indexOf(sub, start - 1);
    auto *ret = new cfvariant(pos >= 0 ? pos + 1 : 0);
    return ret;
}

} // namespace cfml
