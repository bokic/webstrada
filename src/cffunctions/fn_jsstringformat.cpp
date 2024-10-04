/**
 * @file fn_jsstringformat.cpp
 * @brief CFML jsstringformat() built-in.
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

cfvariant *cf_jsstringformat(const cfvariant *str) {
    if (!str) throw webstrada::exception("JSStringFormat requires exactly 1 argument");
    webstrada::string s = variantToString(*str);
    webstrada::string res;
    for (size_t i = 0; i < (size_t)s.length(); i++) {
        char c = s.at(static_cast<int>(i));
        if (c == '\\') res.append("\\\\");
        else if (c == '\'') res.append("\\'");
        else if (c == '"') res.append("\\\"");
        else if (c == '\n') res.append("\\n");
        else if (c == '\r') res.append("\\r");
        else if (c == '\t') res.append("\\t");
        else if (c == '\b') res.append("\\b");
        else if (c == '\f') res.append("\\f");
        else res.append(c);
    }
    auto *ret = new cfvariant(res);
    return ret;
}

} // namespace cfml
