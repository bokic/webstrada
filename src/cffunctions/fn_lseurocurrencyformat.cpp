/**
 * @file fn_lseurocurrencyformat.cpp
 * @brief CFML lseurocurrencyformat() built-in.
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

cfvariant *cf_lseurocurrencyformat(const cfvariant *number, const cfvariant *type, const cfvariant *locale) {
    // Euro-zone locales format in Euro; others format in their local currency.
    const cfml::LocaleInfo *loc = resolveLocale(locale);
    double num = lsNumberValue(number, "LSEuroCurrencyFormat");
    string typeStr = "local";
    if (type && type->m_type != cfvariant::Null) {
        typeStr = const_cast<cfvariant*>(type)->toString();
    }
    const char *euroLocales[] = {"nl","fr","de","it","pt","es"}; // standard euro-zone languages
    bool euro = false;
    for (const char *lang : euroLocales) {
        if (webstrada::string(lang).equals(loc->language)) { euro = true; break; }
    }
    std::string res;
    if (euro) {
        const cfml::LocaleInfo *eur = cfml::locale_find("fr_FR");
        res = formatCurrency(num, typeStr.constData(), eur ? eur : loc);
    } else {
        res = formatCurrency(num, typeStr.constData(), loc);
    }
    auto *ret = new cfvariant(res.c_str());
    return ret;
}

} // namespace cfml
