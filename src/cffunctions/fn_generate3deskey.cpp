/**
 * @file fn_generate3deskey.cpp
 * @brief CFML generate3deskey() built-in.
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

cfvariant *cf_generate3deskey(const cfvariant *seed) {
    if (!seed) throw webstrada::exception("Generate3DesKey requires exactly 1 argument");
    std::vector<std::byte> bytes;
    stringToBytes(variantToString(*seed), "UTF-8", bytes);
    webstrada::string out;
    binaryEncodeBase64(bytes, out, false);
    auto *ret = new cfvariant(out);
    return ret;
}

} // namespace cfml
