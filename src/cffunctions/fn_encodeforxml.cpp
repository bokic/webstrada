/**
 * @file fn_encodeforxml.cpp
 * @brief CFML encodeforxml() built-in.
 */

#include "common.h"

#include "fn_esapi.h"

#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>
#include <webstrada/exceptions.h>
#include <webstrada/string.h>
#include <string>

namespace cfml {

cfvariant *cf_encodeforxml(const cfvariant *str, const cfvariant *canonicalize) {
    if (!str) throw webstrada::exception("EncodeForXML requires 1 argument");
    string input = const_cast<cfvariant*>(str)->toString();
    std::string in = input.constData() ? input.constData() : "";
    if (canonicalize && cf_is_truthy_value(canonicalize)) {
        in = esapiCanonicalizeCatch(in, false, false, false);
    }
    // IMMUNE_XML = {',', '.', '-', '_', ' '}
    static const char kImmune[] = {',', '.', '-', '_', ' '};
    std::string out = esapiEncodeString(in, esapiEncodeXmlCodePoint, kImmune, sizeof(kImmune));
    return new cfvariant(out.c_str());
}

} // namespace cfml
