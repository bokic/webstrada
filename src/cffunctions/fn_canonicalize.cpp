/**
 * @file fn_canonicalize.cpp
 * @brief CFML canonicalize() built-in.
 */

#include "common.h"

#include "fn_esapi.h"

#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>
#include <webstrada/exceptions.h>
#include <webstrada/string.h>
#include <string>

namespace cfml {

cfvariant *cf_canonicalize(const cfvariant *input, const cfvariant *restrictMultiple,
                           const cfvariant *restrictMixed, const cfvariant *throwOnError) {
    if (!input || !restrictMultiple || !restrictMixed) {
        throw webstrada::exception("Canonicalize requires at least 3 arguments");
    }
    string in = const_cast<cfvariant*>(input)->toString();
    std::string str = in.constData() ? in.constData() : "";
    bool rm = cf_is_truthy_value(restrictMultiple);
    bool rmx = cf_is_truthy_value(restrictMixed);
    bool te = throwOnError && cf_is_truthy_value(throwOnError);
    std::string out = esapiCanonicalizeCatch(str, rm, rmx, te);
    return new cfvariant(out.c_str());
}

} // namespace cfml
