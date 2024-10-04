/**
 * @file fn_decrypt.cpp
 * @brief CFML decrypt() built-in.
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

static void decodeCipherBytes(const webstrada::string &in, const webstrada::string &encoding, std::vector<std::byte> &out) {
    webstrada::string enc = encoding;
    enc.toUpper();
    if (enc.isEmpty()) enc = "UU";
    try {
        if (enc.equals("UU")) {
            binaryDecodeUU(in, out);
        } else if (enc.equals("BASE64")) {
            binaryDecodeBase64(in, out, false);
        } else if (enc.equals("HEX")) {
            binaryDecodeHex(in, out);
        } else {
            throw webstrada::exception("Decrypt: Unknown encoding '" + encoding + "'. Supported encodings: UU, Base64, Hex");
        }
    } catch (const webstrada::exception &e) {
        throw webstrada::exception("An error occurred while trying to encrypt or decrypt your input string: " + e.m_message);
    }
}

cfvariant *cf_decrypt(const cfvariant *str, const cfvariant *key, const cfvariant *algorithm, const cfvariant *encoding, const cfvariant *IVorSalt, const cfvariant *iterations) {
    if (!str || !key) throw webstrada::exception("Decrypt requires at least 2 arguments");
    if (iterations) {
        throw webstrada::exception("Decrypt: The iterations parameter is only supported with PBE algorithms, which are not available");
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
    decodeCipherBytes(variantToString(*str), encName, input);

    std::vector<std::byte> iv;
    if (IVorSalt) {
        variantToBytes(IVorSalt, "UTF-8", iv);
    }

    std::vector<std::byte> out;
    cipherDecrypt(input, alg, keyBytes, iv, out);

    webstrada::string result(reinterpret_cast<const char*>(out.data()), out.size());
    auto *ret = new cfvariant(result);
    return ret;
}

} // namespace cfml
