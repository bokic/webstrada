/**
 * @file fn_getpagecontext.cpp
 * @brief CFML getpagecontext() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>
#include <webstrada/exceptions.h>

namespace cfml {

cfvariant *cf_getpagecontext() {
    throw webstrada::exception("Function GetPageContext is not supported: it returns a Java page context object.");
}

} // namespace cfml
