/**
 * @file fn_randomize.cpp
 * @brief CFML randomize() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <climits>

namespace cfml {

cfvariant *cf_randomize(const cfvariant *arg, const cfvariant *algorithm) {
    if (!arg) throw webstrada::exception("Randomize requires 1 or 2 arguments");
    // Unlike Rand/RandRange, CF's Randomize uses the 2-arg
    // getRandomNumberGenerator(String, int seed), which always calls
    // SecureRandom.getInstance and reseeds: it validates the algorithm on
    // EVERY call and never consults the per-thread cache (it does, however,
    // overwrite the cache with the freshly seeded generator).
    std::string algo = randDefaultAlgorithm();
    if (algorithm && algorithm->m_type != cfvariant::Null)
        algo = safe_to_std_string(*algorithm);
    randAlgorithmValidate(algo);
    g_randGenInitialized = true;
    srand(getIntValue(*arg));
    // CF's Randomize returns the next value of the freshly reseeded generator
    // (CFPage.Randomize -> random.nextDouble()): a numeric in [0, 1) that is
    // deterministic for a given seed, and the following Rand()/RandRange() call
    // continues the seeded sequence. Match cf_rand's rendering.
    cfvariant res(cfvariant::Float);
    res.m_double = static_cast<double>(rand()) / RAND_MAX;
    auto *ret = new cfvariant(res);
    return ret;
}

} // namespace cfml
