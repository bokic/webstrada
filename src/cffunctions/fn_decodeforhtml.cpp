/**
 * @file fn_decodeforhtml.cpp
 * @brief CFML decodeforhtml() built-in.
 */

#include "common.h"

#include "fn_esapi.h"

#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>
#include <webstrada/exceptions.h>
#include <webstrada/string.h>
#include <string>

namespace cfml {

cfvariant *cf_decodeforhtml(const cfvariant *str) {
    if (!str) throw webstrada::exception("DecodeForHTML requires exactly 1 argument");
    string input = const_cast<cfvariant*>(str)->toString();
    std::string in = input.constData() ? input.constData() : "";
    std::string out = esapiDecodeHtml(in);
    return new cfvariant(out.c_str());
}

} // namespace cfml
