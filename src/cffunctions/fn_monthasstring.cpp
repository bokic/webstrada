/**
 * @file fn_monthasstring.cpp
 * @brief CFML monthasstring() built-in.
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

cfvariant *cf_monthasstring(const cfvariant *month_number, const cfvariant *locale) {
    if (!month_number) throw webstrada::exception("MonthAsString requires at least 1 argument");
    int mon = getIntValue(*month_number);
    if (mon < 1 || mon > 12) throw webstrada::exception("MonthAsString: Month number must be between 1 and 12.");
    const char *months[] = { "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December" };
    auto *ret = new cfvariant(months[mon - 1]);
    return ret;
}

} // namespace cfml
