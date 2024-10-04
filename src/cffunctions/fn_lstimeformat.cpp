/**
 * @file fn_lstimeformat.cpp
 * @brief CFML lstimeformat() built-in.
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

cfvariant *cf_lstimeformat(const cfvariant *time, const cfvariant *mask, const cfvariant *locale) {
    const cfml::LocaleInfo *loc = resolveLocale(locale);
    double days = getDaysOrThrow(time, "LSTimeFormat");
    string maskStr = "";
    if (mask && mask->m_type != cfvariant::Null) {
        maskStr = const_cast<cfvariant*>(mask)->toString();
    }
    if (maskStr.trimmed().isEmpty()) maskStr = "medium";
    auto *ret = new cfvariant(formatDateTime(days, maskStr, ModeTime, loc));
    return ret;
}

} // namespace cfml
