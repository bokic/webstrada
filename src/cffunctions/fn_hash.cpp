/**
 * @file fn_hash.cpp
 * @brief CFML hash() built-in.
 */

#include "common.h"

#include "../cftags/common.h"
#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>
#include <webstrada/exceptions.h>
#include <webstrada/string.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/hmac.h>
#include <openssl/provider.h>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <strings.h>

using webstrada::cfvariant;
using webstrada::string;

namespace cfml {

cfvariant *cf_hash(const cfvariant *str, const cfvariant *algorithm, const cfvariant *encoding, const cfvariant *additionalIterations) {
    if (!str) throw webstrada::exception("Hash requires at least 1 argument");
    webstrada::string algName = algorithm ? const_cast<cfvariant*>(algorithm)->toString() : "SHA-256";
    webstrada::string norm = algName;
    norm.toUpper();
    if (norm.equals("CFMX_COMPAT")) {
        auto *ret = new cfvariant("");
        return ret;
    }
    const EVP_MD *md = cryptoDigestByName(norm);
    if (!md) {
        throw webstrada::exception("Can't find resource for base name coldfusion/security/resource.properties");
    }
    webstrada::string encName = encoding ? const_cast<cfvariant*>(encoding)->toString() : "UTF-8";
    std::vector<std::byte> input;
    stringToBytes(variantToString(*str), encName, input);

    int iterations = additionalIterations ? getIntValue(*additionalIterations) : 0;
    if (iterations < 0) iterations = 0;

    std::vector<unsigned char> digest(static_cast<size_t>(EVP_MAX_MD_SIZE));
    unsigned int digestLen = 0;
    std::vector<std::byte> data = input;
    loadCryptoProviders();
    for (int i = 0; i <= iterations; i++) {
        digestLen = 0;
        EVP_MD_CTX *ctx = EVP_MD_CTX_new();
        if (!ctx) throw webstrada::exception("Hash: out of memory");
        int rc = EVP_DigestInit_ex(ctx, md, nullptr);
        if (rc == 1) rc = EVP_DigestUpdate(ctx, data.data(), data.size());
        if (rc == 1) rc = EVP_DigestFinal_ex(ctx, digest.data(), &digestLen);
        EVP_MD_CTX_free(ctx);
        if (rc != 1) {
            throw webstrada::exception("Hash: digest computation failed");
        }
        // Feed the raw digest bytes back into the next iteration.
        data.assign(reinterpret_cast<std::byte*>(digest.data()), reinterpret_cast<std::byte*>(digest.data()) + digestLen);
    }

    webstrada::string result = uppercaseHex(digest.data(), digestLen);
    auto *ret = new cfvariant(result);
    return ret;
}

} // namespace cfml
