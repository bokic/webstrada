/**
 * @file fn_hmac.cpp
 * @brief CFML hmac() built-in.
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

cfvariant *cf_hmac(const cfvariant *message, const cfvariant *key, const cfvariant *algorithm, const cfvariant *encoding) {
    if (!message || !key) throw webstrada::exception("HMac requires at least 2 arguments");
    webstrada::string algName = algorithm ? const_cast<cfvariant*>(algorithm)->toString() : "HMACMD5";
    const EVP_MD *md = cryptoDigestByName(algName);
    if (!md) {
        throw webstrada::exception("Can't find resource for base name coldfusion/security/resource.properties");
    }
    webstrada::string encName = encoding ? const_cast<cfvariant*>(encoding)->toString() : "UTF-8";
    std::vector<std::byte> msg;
    variantToBytes(message, encName, msg);
    std::vector<std::byte> keyBytes;
    variantToBytes(key, encName, keyBytes);
    if (keyBytes.empty()) {
        auto *ret = new cfvariant("");
        return ret;
    }
    loadCryptoProviders();
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digestLen = 0;
    if (!HMAC(md, keyBytes.data(), static_cast<int>(keyBytes.size()),
        reinterpret_cast<const unsigned char*>(msg.data()), msg.size(),
        digest, &digestLen)) {
        throw webstrada::exception("HMac: computation failed");
    }
    webstrada::string result = uppercaseHex(digest, digestLen);
    auto *ret = new cfvariant(result);
    return ret;
}

} // namespace cfml
