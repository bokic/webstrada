/**
 * @file fn_left.cpp
 * @brief CFML left() built-in.
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

cfvariant *cf_left(const cfvariant *str, const cfvariant *cnt) {
    if (!str || !cnt) throw webstrada::exception("Left requires exactly 2 arguments");
    webstrada::string s = const_cast<cfvariant*>(str)->toString();
    int count = (cnt->m_type == cfvariant::Number) ? cnt->m_int : atoi(const_cast<cfvariant*>(cnt)->toString().constData());
    if (count < 0) throw webstrada::exception("Left: count cannot be negative");
    auto *ret = new cfvariant(s.left(count));
    return ret;
}

} // namespace cfml
