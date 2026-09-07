/**
 * @file fn_setencoding.cpp
 * @brief CFML setencoding() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>
#include <webstrada/exceptions.h>
#include <webstrada/string.h>
#include <string>

namespace cfml {

cfvariant *cf_setencoding(const cfvariant *scope, const cfvariant *encoding) {
    if (!scope || !encoding) {
        throw webstrada::exception("SetEncoding requires exactly 2 arguments");
    }
    webstrada::string scopeName = const_cast<cfvariant*>(scope)->toString();
    scopeName.toUpper();
    if (scopeName.equals("FORM")) {
        set_form_encoding(const_cast<cfvariant*>(encoding)->toString().constData());
        return nullptr;
    }
    if (scopeName.equals("URL")) {
        set_url_encoding(const_cast<cfvariant*>(encoding)->toString().constData());
        return nullptr;
    }
    throw webstrada::exception("Only form or URL scope is allowed in a SetEncoding() call.");
}

} // namespace cfml
