/**
 * @file fn_lsisdate.cpp
 * @brief CFML lsisdate() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>
#include <webstrada/exceptions.h>
#include <webstrada/locale.h>
#include <webstrada/string.h>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <string>
#include <vector>
#include <algorithm>

namespace cfml {

cfvariant *cf_lsisdate(const cfvariant *stringVal, const cfvariant *locale) {
    if (!stringVal) throw webstrada::exception("LSIsDate requires exactly 1 argument");
    const cfml::LocaleInfo *loc = resolveLocale(locale);
    cfvariant res(cfvariant::Boolean);
    if (stringVal->m_type == cfvariant::DateTime) {
        res.m_bool = true;
    } else if (stringVal->m_type != cfvariant::String) {
        // CF: LSIsDate(non-string) is always false (numbers are handled below).
        res.m_bool = false;
    } else {
        // CF: number-only strings (all digits) are never dates.
        webstrada::string s = const_cast<cfvariant*>(stringVal)->toString();
        const char *cs = s.constData();
        size_t n = s.length();
        size_t i = 0;
        while (i < n && std::isdigit((unsigned char)cs[i])) i++;
        if (i == n) {
            res.m_bool = false;
        } else {
            double dummy = 0.0;
            res.m_bool = parseDateTimeLocale(s, loc, dummy);
        }
    }
    auto *ret = new cfvariant(res);
    return ret;
}

} // namespace cfml
