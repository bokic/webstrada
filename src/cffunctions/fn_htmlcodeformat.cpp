/**
 * @file fn_htmlcodeformat.cpp
 * @brief CFML htmlcodeformat() built-in.
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

cfvariant *cf_htmlcodeformat(const cfvariant *str, const cfvariant *version) {
    if (!str) throw webstrada::exception("HTMLCodeFormat requires at least 1 argument");
    webstrada::string res = "<PRE>";
    res.append(escapeHtmlEdit(variantToString(*str)));
    res.append("</PRE>");
    auto *ret = new cfvariant(res);
    return ret;
}

} // namespace cfml
