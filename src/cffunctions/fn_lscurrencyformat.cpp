/**
 * @file fn_lscurrencyformat.cpp
 * @brief CFML lscurrencyformat() built-in.
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

cfvariant *cf_lscurrencyformat(const cfvariant *number, const cfvariant *type, const cfvariant *locale) {
    const cfml::LocaleInfo *loc = resolveLocale(locale);
    double num = lsNumberValue(number, "LSCurrencyFormat");
    string typeStr = "local";
    if (type && type->m_type != cfvariant::Null) {
        typeStr = const_cast<cfvariant*>(type)->toString();
    }
    std::string res = formatCurrency(num, typeStr.constData(), loc);
    auto *ret = new cfvariant(res.c_str());
    return ret;
}

} // namespace cfml
