/**
 * @file fn_lsdatetimeformat.cpp
 * @brief CFML lsdatetimeformat() built-in.
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

cfvariant *cf_lsdatetimeformat(const cfvariant *date, const cfvariant *mask, const cfvariant *locale) {
    const cfml::LocaleInfo *loc = resolveLocale(locale);
    double days = getDaysOrThrow(date, "LSDateTimeFormat");
    string maskStr = "";
    if (mask && mask->m_type != cfvariant::Null) {
        maskStr = const_cast<cfvariant*>(mask)->toString();
    }
    if (maskStr.trimmed().isEmpty()) maskStr = "dd-MMM-yyyy HH:nn:ss";
    auto *ret = new cfvariant(formatDateTime(days, maskStr, ModeDateTime, loc));
    return ret;
}

} // namespace cfml
