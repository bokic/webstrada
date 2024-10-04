/**
 * @file fn_lsparsecurrency.cpp
 * @brief CFML lsparsecurrency() built-in.
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

cfvariant *cf_lsparsecurrency(const cfvariant *stringVal, const cfvariant *locale) {
    const cfml::LocaleInfo *loc = resolveLocale(locale);
    if (!stringVal) throw webstrada::exception("LSParseCurrency requires at least 1 argument");
    string s = const_cast<cfvariant*>(stringVal)->toString();
    // strip the currency symbol / intl code and any parentheses / spaces
    std::string cleaned = s.constData();
    std::string sym(loc->curSymbol);
    size_t p;
    while ((p = cleaned.find(sym)) != std::string::npos) cleaned.erase(p, sym.length());
    std::string intl(loc->curIntl);
    while ((p = cleaned.find(intl)) != std::string::npos) cleaned.erase(p, intl.length());
    cleaned.erase(std::remove(cleaned.begin(), cleaned.end(), '('), cleaned.end());
    cleaned.erase(std::remove(cleaned.begin(), cleaned.end(), ')'), cleaned.end());
    cleaned.erase(std::remove(cleaned.begin(), cleaned.end(), ' '), cleaned.end());
    if (cleaned.find("--") != std::string::npos) {
        throw webstrada::exception("The value " + s + " cannot be parsed as a currency value.");
    }
    double v = 0.0;
    if (!parseNumberWithLocale(string(cleaned.c_str()), loc, v)) {
        throw webstrada::exception("The value " + s + " cannot be parsed as a currency value.");
    }
    cfvariant rv(cfvariant::Float); rv.m_double = v; auto *ret = new cfvariant(rv);
    return ret;
}

} // namespace cfml
