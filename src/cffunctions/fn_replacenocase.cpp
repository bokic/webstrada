/**
 * @file fn_replacenocase.cpp
 * @brief CFML replacenocase() built-in.
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

cfvariant *cf_replacenocase(const cfvariant *str, const cfvariant *sub1Val, const cfvariant *sub2Val, const cfvariant *scopeVal) {
    if (!str || !sub1Val || !sub2Val) throw webstrada::exception("ReplaceNoCase requires at least 3 arguments");
    webstrada::string s = const_cast<cfvariant*>(str)->toString();
    webstrada::string sub1 = const_cast<cfvariant*>(sub1Val)->toString();
    webstrada::string sub2 = const_cast<cfvariant*>(sub2Val)->toString();
    webstrada::string scope = "ONE";
    if (scopeVal) {
        scope = const_cast<cfvariant*>(scopeVal)->toString();
    }
    bool all = (scope.compareCaseInsensitive("ALL") == 0);

    if (sub1.isEmpty()) {
        auto *ret = new cfvariant(s);
        return ret;
    }

    webstrada::string sLower = s; sLower.toLower();
    webstrada::string sub1Lower = sub1; sub1Lower.toLower();

    webstrada::string res;
    int start = 0;
    while (start < s.length()) {
        int pos = sLower.indexOf(sub1Lower, start);
        if (pos < 0) {
            res += s.mid(start, s.length() - start);
            break;
        }
        res += s.mid(start, pos - start);
        res += sub2;
        start = pos + sub1.length();
        if (!all) {
            res += s.mid(start, s.length() - start);
            break;
        }
    }
    auto *ret = new cfvariant(res);
    return ret;
}

} // namespace cfml
