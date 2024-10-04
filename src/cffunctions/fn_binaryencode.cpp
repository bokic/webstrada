/**
 * @file fn_binaryencode.cpp
 * @brief CFML binaryencode() built-in.
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

cfvariant *cf_binaryencode(const cfvariant *binaryData, const cfvariant *encoding) {
    if (!binaryData || !encoding) throw webstrada::exception("BinaryEncode requires exactly 2 arguments");
    if (binaryData->m_type != cfvariant::Binary || !binaryData->m_binary) {
        throw webstrada::exception("BinaryEncode: The first argument must be a binary value");
    }
    webstrada::string enc = const_cast<cfvariant*>(encoding)->toString();
    enc.toUpper();
    webstrada::string out;
    if (enc.equals("HEX")) {
        binaryEncodeHex(*binaryData->m_binary, out);
    } else if (enc.equals("BASE64")) {
        binaryEncodeBase64(*binaryData->m_binary, out, false);
    } else if (enc.equals("BASE64URL")) {
        binaryEncodeBase64(*binaryData->m_binary, out, true);
    } else if (enc.equals("UU")) {
        binaryEncodeUU(*binaryData->m_binary, out);
    } else {
        throw webstrada::exception("BinaryEncode: Unknown encoding '" + enc + "'. Supported encodings: hex, UU, base64, base64URL");
    }
    auto *ret = new cfvariant(out);
    return ret;
}

} // namespace cfml
