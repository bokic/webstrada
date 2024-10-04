/**
 * @file fn_lsparsenumber.cpp
 * @brief CFML lsparsenumber() built-in.
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

cfvariant *cf_lsparsenumber(const cfvariant *stringVal, const cfvariant *locale) {
    const cfml::LocaleInfo *loc = resolveLocale(locale);
    if (!stringVal) throw webstrada::exception("LSParseNumber requires at least 1 argument");
    string s = const_cast<cfvariant*>(stringVal)->toString();
    double v = 0.0;
    if (!parseNumberWithLocale(s, loc, v)) {
        throw webstrada::exception("The value " + s + " cannot be parsed as a number.");
    }
    cfvariant rv(cfvariant::Float); rv.m_double = v; auto *ret = new cfvariant(rv);
    return ret;
}

} // namespace cfml
