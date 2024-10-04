/**
 * @file fn_isbinary.cpp
 * @brief CFML isbinary() built-in.
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

cfvariant *cf_isbinary(const cfvariant *val) {
    auto *ret = new cfvariant(cfvariant::Boolean);
    // A cfhttp ByteArrayOutputStream (getasbinary="no" + non-text MIME) is NOT
    // a byte[] so IsBinary reports NO (was BUGS.md "cfhttp getasbinary=no
    // stores binary").
    ret->m_bool = val && (val->m_type == cfvariant::Binary) && !val->m_isByteArrayOutputStream;
    return ret;
}

} // namespace cfml
