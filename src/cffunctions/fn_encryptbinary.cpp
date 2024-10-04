/**
 * @file fn_encryptbinary.cpp
 * @brief CFML encryptbinary() built-in.
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

cfvariant *cf_encryptbinary(const cfvariant *binaryData, const cfvariant *key, const cfvariant *algorithm, const cfvariant *encoding, const cfvariant *IVorSalt, const cfvariant *iterations) {
    if (!binaryData || !key) throw webstrada::exception("EncryptBinary requires at least 2 arguments");
    if (iterations) {
        throw webstrada::exception("EncryptBinary: The iterations parameter is only supported with PBE algorithms, which are not available");
    }
    webstrada::string algName = algorithm ? variantToString(*algorithm) : "CFMX_COMPAT";
    webstrada::string encName = encoding ? variantToString(*encoding) : "UU";
    encName.toUpper();
    if (!encName.equals("UU") && !encName.equals("BASE64") && !encName.equals("HEX")) {
        throw webstrada::exception("EncryptBinary: Unknown encoding '" + encName + "'. Supported encodings: UU, Base64, Hex");
    }

    CipherAlg alg;
    if (!parseCipherAlgorithm(algName, alg)) {
        throw webstrada::exception("The " + algName + " algorithm is not supported by the Security Provider you have chosen.");
    }

    std::vector<std::byte> keyBytes;
    decodeEncryptKey(variantToString(*key), keyBytes);

    std::vector<std::byte> input;
    variantToBytes(binaryData, "UTF-8", input);

    std::vector<std::byte> iv;
    bool prependIv = false;
    if (IVorSalt) {
        variantToBytes(IVorSalt, "UTF-8", iv);
    } else {
        prependIv = true;
    }

    std::vector<std::byte> out;
    cipherEncrypt(input, alg, keyBytes, iv, prependIv, out);

    auto *ret = new cfvariant(cfvariant::Binary);
    *ret->m_binary = std::move(out);
    return ret;
}

} // namespace cfml
