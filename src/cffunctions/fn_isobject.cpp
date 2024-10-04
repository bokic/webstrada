/**
 * @file fn_isobject.cpp
 * @brief CFML isobject() built-in.
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

cfvariant *cf_isobject(const cfvariant *val) {
    auto *ret = new cfvariant(cfvariant::Boolean);
    // A bare built-in function reference is a method-handle object (CFPageMethod)
    // in CF, so IsObject is true for it too (verified against CF 2021).
    ret->m_bool = val && (val->m_type == cfvariant::Component || val->m_type == cfvariant::Function);
    return ret;
}

} // namespace cfml
