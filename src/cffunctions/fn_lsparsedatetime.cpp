/**
 * @file fn_lsparsedatetime.cpp
 * @brief CFML lsparsedatetime() built-in.
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

cfvariant *cf_lsparsedatetime(const cfvariant *stringVal, const cfvariant *locale) {
    const cfml::LocaleInfo *loc = resolveLocale(locale);
    if (!stringVal) throw webstrada::exception("LSParseDateTime requires at least 1 argument");
    // A DateTime object passes through unchanged (CF's LSParseDateTime(Object, ...)).
    if (stringVal->m_type == cfvariant::DateTime) {
        return new cfvariant(*stringVal);
    }
    double days = 0.0;
    if (!parseDateTimeLocale(const_cast<cfvariant*>(stringVal)->toString(), loc, days)) {
        throw webstrada::exception("LSParseDateTime: Invalid date/time value");
    }
    cfvariant res(cfvariant::DateTime);
    res.m_double = days;
    auto *ret = new cfvariant(res);
    return ret;
}

} // namespace cfml
