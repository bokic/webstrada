/**
 * @file fn_isnumeric.cpp
 * @brief CFML isnumeric() built-in.
 */

#include "common.h"

#include "../cftags/common.h"
#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>
#include <webstrada/exceptions.h>
#include <webstrada/scope_store.h>
#include <webstrada/string.h>
#include <charconv>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <string>
#include <thread>
#include <chrono>
#include <vector>
#include <unistd.h>

using webstrada::cfvariant;
using webstrada::string;
using cfml::daysToTm;
using cfml::getIntValue;
using cfml::isTruthy;
using cfml::safe_to_std_string;
using cfml::variantToString;
using cfml::cfvariant_to_long;
using cfml::normalizeCharsetName;
using cfml::bytesToText;
using cfml::urlDecodeString;
using cfml::stringToBytes;
using cfml::getDaysOrThrow;
using cfml::tmToDays;
using cfml::cryptoHexDigits;

namespace cfml {

static bool canParseAsDouble(const string &s) {
    if (s.isEmpty()) return false;
    char *endptr = nullptr;
    const char *str = s.constData();
    while (*str && std::isspace(static_cast<unsigned char>(*str))) str++;
    if (!*str) return false;
    std::strtod(str, &endptr);
    while (endptr && *endptr && std::isspace(static_cast<unsigned char>(*endptr))) endptr++;
    return endptr && (*endptr == '\0');
}

cfvariant *cf_isnumeric(const cfvariant *val) {
    auto *ret = new cfvariant(cfvariant::Boolean);
    ret->m_bool = false;
    if (val) {
        // A q.col query-column reference is stored as an Array wrapper;
        // unwrap to the underlying cell before the type check.
        cfvariant scalar;
        if (val->m_type == cfvariant::Array && val->m_queryColOwner) {
            scalar = queryColumnFirstCell(val);
            val = &scalar;
        }
        if (val->m_type == cfvariant::Number || val->m_type == cfvariant::Float ||
            val->m_type == cfvariant::Long) {
            ret->m_bool = true;
        } else if (val->m_type == cfvariant::String) {
            ret->m_bool = canParseAsDouble(const_cast<cfvariant*>(val)->toString());
        }
    }
    return ret;
}

} // namespace cfml
