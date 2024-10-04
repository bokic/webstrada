/**
 * @file fn_tobinary.cpp
 * @brief CFML tobinary() built-in.
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

cfvariant *cf_tobinary(const cfvariant *input) {
    if (!input) throw webstrada::exception("ToBinary requires exactly 1 argument");
    std::vector<std::byte> data;
    if (input->m_type == cfvariant::Binary && input->m_binary) {
        data = *input->m_binary;
    } else {
        webstrada::string s = variantToString(*input);
        try {
            binaryDecodeBase64(s, data, false);
        } catch (const webstrada::exception &e) {
            throw webstrada::exception("ToBinary: Cannot decode the base64 string: " + e.m_message);
        }
    }
    auto *ret = new cfvariant(cfvariant::Binary);
    *ret->m_binary = std::move(data);
    return ret;
}

} // namespace cfml
