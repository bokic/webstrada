/**
 * @file fn_binarydecode.cpp
 * @brief CFML binarydecode() built-in.
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

cfvariant *cf_binarydecode(const cfvariant *str, const cfvariant *encoding) {
    if (!str || !encoding) throw webstrada::exception("BinaryDecode: Missing argument");

    webstrada::string s = const_cast<cfvariant*>(str)->toString();
    webstrada::string enc = const_cast<cfvariant*>(encoding)->toString();
    enc.toUpper();

    std::vector<std::byte> data;
    if (enc.equals("HEX")) {
        binaryDecodeHex(s, data);
    } else if (enc.equals("BASE64")) {
        binaryDecodeBase64(s, data, false);
    } else if (enc.equals("BASE64URL")) {
        binaryDecodeBase64(s, data, true);
    } else if (enc.equals("UU")) {
        binaryDecodeUU(s, data);
    } else {
        throw webstrada::exception("BinaryDecode: Unknown encoding '" + enc + "'. Supported encodings: hex, UU, base64, base64URL");
    }

    auto *ret = new cfvariant(cfvariant::Binary);
    *ret->m_binary = std::move(data);
    return ret;
}

} // namespace cfml
