/**
 * @file fn_setlocale.cpp
 * @brief CFML setlocale() built-in.
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

cfvariant *cf_setlocale(const cfvariant *locale) {
    if (!locale || locale->m_type == cfvariant::Null) {
        throw webstrada::exception("SetLocale requires 1 argument");
    }
    string name = const_cast<cfvariant*>(locale)->toString();
    const cfml::LocaleInfo *loc = cfml::locale_find(name.constData());
    if (!loc) {
        throw webstrada::exception("The locale, " + name + ", cannot be found.");
    }
    // SetLocale returns the *previous* locale's CF string (verified on CF).
    webstrada::string old = currentLocaleStr();
    g_currentLocale = loc;
    g_currentLocaleStr = loc->cfName;

    // Set the Content-Language response header (lang-COUNTRY) and, like CF,
    // adjust the response charset unless one was explicitly requested.
    auto &r = response();
    webstrada::string cl = loc->language;
    cl += "-";
    cl += loc->country;
    r.contentLanguage = cl;

    auto *ret = new cfvariant(old);
    return ret;
}

} // namespace cfml
