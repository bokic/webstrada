/**
 * @file fn_duplicate.cpp
 * @brief CFML duplicate() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>
#include <webstrada/exceptions.h>
#include <webstrada/string.h>

namespace cfml {

cfvariant *cf_duplicate(const cfvariant *obj) {
    if (!obj) throw webstrada::exception("Duplicate requires exactly 1 argument");
    // cfvariant::deepCopy() is a fully independent recursive clone (StructCopy
    // / Duplicate semantics), used by StructCopy and Duplicate.
    cfvariant copy = const_cast<cfvariant*>(obj)->deepCopy();
    auto *ret = new cfvariant(copy);
    return ret;
}

} // namespace cfml
