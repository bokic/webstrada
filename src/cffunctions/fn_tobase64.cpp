/**
 * @file fn_tobase64.cpp
 * @brief CFML tobase64() built-in.
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

cfvariant *cf_tobase64(const cfvariant *input, const cfvariant *encoding) {
    if (!input) throw webstrada::exception("ToBase64 requires at least 1 argument");
    std::vector<std::byte> bytes;
    if (input->m_type == cfvariant::Binary && input->m_binary) {
        bytes = *input->m_binary;
    } else {
        webstrada::string encName = encoding ? const_cast<cfvariant*>(encoding)->toString() : "UTF-8";
        stringToBytes(variantToString(*input), encName, bytes);
    }
    webstrada::string out;
    binaryEncodeBase64(bytes, out, false);
    auto *ret = new cfvariant(out);
    return ret;
}

} // namespace cfml
