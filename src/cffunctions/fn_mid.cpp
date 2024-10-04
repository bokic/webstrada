/**
 * @file fn_mid.cpp
 * @brief CFML mid() built-in.
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

cfvariant *cf_mid(const cfvariant *str, const cfvariant *startVal, const cfvariant *cnt) {
    if (!str || !startVal || !cnt) throw webstrada::exception("Mid requires exactly 3 arguments");
    webstrada::string s = const_cast<cfvariant*>(str)->toString();
    int start = (startVal->m_type == cfvariant::Number) ? startVal->m_int : atoi(const_cast<cfvariant*>(startVal)->toString().constData());
    int count = (cnt->m_type == cfvariant::Number) ? cnt->m_int : atoi(const_cast<cfvariant*>(cnt)->toString().constData());
    if (start < 1) throw webstrada::exception("Mid: start position must be 1 or greater");
    if (count < 0) throw webstrada::exception("Mid: count cannot be negative");
    auto *ret = new cfvariant(s.mid(start - 1, count));
    return ret;
}

} // namespace cfml
