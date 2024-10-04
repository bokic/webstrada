/**
 * @file fn_randrange.cpp
 * @brief CFML randrange() built-in.
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

cfvariant *cf_randrange(const cfvariant *number1, const cfvariant *number2, const cfvariant *algorithm) {
    if (!number1 || !number2) throw webstrada::exception("RandRange requires exactly 2 or 3 arguments");
    // CF caches its SecureRandom generator in a per-thread ThreadLocal
    // (SecureRandomGenerator.randRef); the algorithm is validated only on the
    // *first* Rand/RandRange call of a thread. Later calls reuse the cached
    // generator and silently ignore a bad algorithm name — the cache is never
    // cleared, so this persists across templates/requests on the same pooled
    // thread. Our generator is rand(); the validation mirrors CF's timing
    // while the actual randomness stays the same engine-wide.
    if (!g_randGenInitialized) {
        std::string algo = randDefaultAlgorithm();
        if (algorithm && algorithm->m_type != cfvariant::Null)
            algo = safe_to_std_string(*algorithm);
        randAlgorithmValidate(algo);
        g_randGenInitialized = true;
    }
    // CF swaps the bounds when number1 > number2 (verified: RandRange(5,3)
    // yields values in [3,5]) and returns an integer in [min, max] inclusive.
    long long lo = cfvariant_to_long(number1);
    long long hi = cfvariant_to_long(number2);
    if (lo > hi) std::swap(lo, hi);
    long long span = hi - lo + 1;
    long long result = lo;
    if (span > 0) {
        double d = static_cast<double>(rand()) / RAND_MAX;   // matches cf_rand
        result = lo + static_cast<long long>(d * static_cast<double>(span));
        if (result > hi) result = hi;
    }
    cfvariant *ret = new cfvariant(static_cast<int>(result));
    return ret;
}

} // namespace cfml
