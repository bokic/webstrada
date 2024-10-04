/**
 * @file fn_rand.cpp
 * @brief CFML rand() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <climits>

namespace cfml {

cfvariant *cf_rand(const cfvariant *algorithm) {
    // Same cached path as RandRange: CF's per-thread SecureRandom generator is
    // validated only on the first Rand/RandRange call of a thread; later calls
    // silently ignore a bad algorithm name.
    if (!g_randGenInitialized) {
        std::string algo = randDefaultAlgorithm();
        if (algorithm && algorithm->m_type != cfvariant::Null)
            algo = safe_to_std_string(*algorithm);
        randAlgorithmValidate(algo);
        g_randGenInitialized = true;
    }
    cfvariant res(cfvariant::Float);
    res.m_double = static_cast<double>(rand()) / RAND_MAX;
    auto *ret = new cfvariant(res);
    return ret;
}

} // namespace cfml
