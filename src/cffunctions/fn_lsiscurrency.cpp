/**
 * @file fn_lsiscurrency.cpp
 * @brief CFML lsiscurrency() built-in.
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

cfvariant *cf_lsiscurrency(const cfvariant *stringVal, const cfvariant *locale) {
    const cfml::LocaleInfo *loc = resolveLocale(locale);
    cfvariant res(cfvariant::Boolean);
    if (!stringVal || stringVal->m_type == cfvariant::Null) {
        res.m_bool = false;
    } else {
        string s = const_cast<cfvariant*>(stringVal)->toString();
        // strip the currency symbol / intl code, then parse the remainder
        std::string cleaned = s.constData();
        std::string sym(loc->curSymbol);
        std::string intl(loc->curIntl);
        size_t p;
        while ((p = cleaned.find(sym)) != std::string::npos) cleaned.erase(p, sym.length());
        while ((p = cleaned.find(intl)) != std::string::npos) cleaned.erase(p, intl.length());
        double v = 0.0;
        res.m_bool = parseNumberWithLocale(string(cleaned.c_str()), loc, v);
    }
    auto *ret = new cfvariant(res);
    return ret;
}

} // namespace cfml
