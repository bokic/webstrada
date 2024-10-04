/**
 * @file fn_generatesecretkey.cpp
 * @brief CFML generatesecretkey() built-in.
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

cfvariant *cf_generatesecretkey(const cfvariant *algorithm, const cfvariant *keySize) {
    if (!algorithm) throw webstrada::exception("GenerateSecretKey requires the algorithm argument");
    webstrada::string algName = variantToString(*algorithm);
    webstrada::string norm = algName;
    norm.toUpper();
    int bits = 0;
    int bytes = 0;
    if (norm.equals("AES")) {
        bits = keySize ? getIntValue(*keySize) : 256;
        if (bits != 128 && bits != 192 && bits != 256) {
            throw webstrada::exception("Wrong keysize: must be equal to 128, 192 or 256");
        }
        bytes = bits / 8;
    } else if (norm.equals("BLOWFISH")) {
        bits = keySize ? getIntValue(*keySize) : 128;
        if (bits < 32 || bits > 448 || (bits % 8) != 0) {
            throw webstrada::exception("Wrong keysize: must be between 32 and 448 bits");
        }
        bytes = bits / 8;
    } else if (norm.equals("DES")) {
        if (keySize && getIntValue(*keySize) != 64) {
            throw webstrada::exception("Wrong keysize: must be equal to 64");
        }
        bytes = 8;
    } else if (norm.equals("DESEDE") || norm.equals("3DES")) {
        bits = keySize ? getIntValue(*keySize) : 168;
        if (bits != 112 && bits != 168) {
            throw webstrada::exception("Wrong keysize: must be equal to 112 or 168");
        }
        bytes = (bits == 112) ? 16 : 24;
    } else {
        throw webstrada::exception("The " + algName + " algorithm is not supported by the Security Provider you have chosen.");
    }
    std::vector<std::byte> key(static_cast<size_t>(bytes));
    loadCryptoProviders();
    if (RAND_bytes(reinterpret_cast<unsigned char*>(key.data()), bytes) != 1) {
        throw webstrada::exception("GenerateSecretKey: failed to generate random key");
    }
    webstrada::string result;
    binaryEncodeBase64(key, result, false);
    auto *ret = new cfvariant(result);
    return ret;
}

} // namespace cfml
