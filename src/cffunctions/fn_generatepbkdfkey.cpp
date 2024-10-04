/**
 * @file fn_generatepbkdfkey.cpp
 * @brief CFML generatepbkdfkey() built-in.
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

cfvariant *cf_generatepbkdfkey(const cfvariant *algorithm, const cfvariant *passphrase, const cfvariant *salt, const cfvariant *iterations, const cfvariant *keySize) {
    if (!algorithm || !passphrase || !salt || !iterations || !keySize) {
        throw webstrada::exception("GeneratePBKDFKey requires 5 arguments");
    }
    webstrada::string algName = variantToString(*algorithm);
    webstrada::string norm = algName;
    norm.toUpper();
    if (norm.startWith("PBKDF2WITH")) norm = norm.mid(10, norm.length() - 10);
    const EVP_MD *md = cryptoDigestByName(norm);
    if (!md) {
        throw webstrada::exception("The " + algName + " algorithm is not supported by the Security Provider you have chosen.");
    }
    std::vector<std::byte> pass;
    stringToBytes(variantToString(*passphrase), "UTF-8", pass);
    std::vector<std::byte> saltBytes;
    stringToBytes(variantToString(*salt), "UTF-8", saltBytes);
    int iter = getIntValue(*iterations);
    int bits = getIntValue(*keySize);
    if (bits <= 0 || (bits % 8) != 0) {
        throw webstrada::exception("GeneratePBKDFKey: keySize must be a positive multiple of 8");
    }
    int keyLen = bits / 8;
    std::vector<std::byte> out(static_cast<size_t>(keyLen));
    loadCryptoProviders();
    int rc = PKCS5_PBKDF2_HMAC(
        reinterpret_cast<const char*>(pass.data()), static_cast<int>(pass.size()),
        reinterpret_cast<const unsigned char*>(saltBytes.data()), static_cast<int>(saltBytes.size()),
        iter, md, keyLen, reinterpret_cast<unsigned char*>(out.data()));
    if (rc != 1) {
        throw webstrada::exception("GeneratePBKDFKey: key derivation failed");
    }
    webstrada::string result;
    binaryEncodeBase64(out, result, false);
    auto *ret = new cfvariant(result);
    return ret;
}

} // namespace cfml
