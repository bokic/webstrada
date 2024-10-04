/**
 * @file fn_getexception.cpp
 * @brief CFML getexception() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>
#include <webstrada/exceptions.h>
#include <webstrada/string.h>

namespace cfml {

cfvariant *cf_getexception(const cfvariant *object) {
    if (!object) {
        throw webstrada::exception("Parameter validation error for GetException function.");
    }
    // CF 2025: GetException(Object) pops the argument from the internal
    // ExceptionCache, which only ever holds real Java exception/proxy objects.
    // CFML values (strings, numbers, structs, even the cfcatch struct) are
    // never registered there, so the result is always null and renders empty.
    // This engine has no Java objects / ExceptionCache, so the faithful
    // CFML-visible behavior for every possible argument is the same null.
    return nullResult();
}

} // namespace cfml
