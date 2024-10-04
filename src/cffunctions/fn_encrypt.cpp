/**
 * @file fn_encrypt.cpp
 * @brief CFML encrypt() built-in.
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

static void encodeCipherBytes(const std::vector<std::byte> &in, const webstrada::string &encoding, webstrada::string &out) {
    webstrada::string enc = encoding;
    enc.toUpper();
    if (enc.isEmpty()) enc = "UU";
    if (enc.equals("UU")) {
        binaryEncodeUU(in, out);
    } else if (enc.equals("BASE64")) {
        binaryEncodeBase64(in, out, false);
    } else if (enc.equals("HEX")) {
        binaryEncodeHex(in, out);
    } else {
        throw webstrada::exception("Encrypt: Unknown encoding '" + encoding + "'. Supported encodings: UU, Base64, Hex");
    }
}

cfvariant *cf_encrypt(const cfvariant *str, const cfvariant *key, const cfvariant *algorithm, const cfvariant *encoding, const cfvariant *IVorSalt, const cfvariant *iterations) {
    if (!str || !key) throw webstrada::exception("Encrypt requires at least 2 arguments");
    if (iterations) {
        throw webstrada::exception("Encrypt: The iterations parameter is only supported with PBE algorithms, which are not available");
    }
    webstrada::string algName = algorithm ? variantToString(*algorithm) : "CFMX_COMPAT";
    webstrada::string encName = encoding ? variantToString(*encoding) : "UU";

    CipherAlg alg;
    if (!parseCipherAlgorithm(algName, alg)) {
        throw webstrada::exception("The " + algName + " algorithm is not supported by the Security Provider you have chosen.");
    }

    std::vector<std::byte> keyBytes;
    decodeEncryptKey(variantToString(*key), keyBytes);

    std::vector<std::byte> input;
    stringToBytes(variantToString(*str), "UTF-8", input);

    std::vector<std::byte> iv;
    bool prependIv = false;
    if (IVorSalt) {
        variantToBytes(IVorSalt, "UTF-8", iv);
    } else {
        prependIv = true;
    }

    std::vector<std::byte> out;
    cipherEncrypt(input, alg, keyBytes, iv, prependIv, out);

    webstrada::string result;
    encodeCipherBytes(out, encName, result);
    auto *ret = new cfvariant(result);
    return ret;
}

} // namespace cfml
