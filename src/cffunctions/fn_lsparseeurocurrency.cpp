/**
 * @file fn_lsparseeurocurrency.cpp
 * @brief CFML lsparseeurocurrency() built-in.
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

cfvariant *cf_lsparseeurocurrency(const cfvariant *stringVal, const cfvariant *locale) {
    const cfml::LocaleInfo *loc = resolveLocale(locale);
    if (!stringVal) throw webstrada::exception("LSParseEuroCurrency requires at least 1 argument");
    string s = const_cast<cfvariant*>(stringVal)->toString();
    std::string cleaned = s.constData();
    for (const char *sym : {"\xE2\x82\xAC", "EUR", "euro"}) {
        std::string ss(sym);
        size_t p;
        while ((p = cleaned.find(ss)) != std::string::npos) cleaned.erase(p, ss.length());
    }
    cleaned.erase(std::remove(cleaned.begin(), cleaned.end(), ' '), cleaned.end());
    double v = 0.0;
    if (!parseNumberWithLocale(string(cleaned.c_str()), loc, v)) {
        throw webstrada::exception("The value " + s + " cannot be parsed as a currency value.");
    }
    cfvariant rv(cfvariant::Float); rv.m_double = v; auto *ret = new cfvariant(rv);
    return ret;
}

} // namespace cfml
