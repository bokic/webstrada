/**
 * @file fn_structcopy.cpp
 * @brief CFML structcopy() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <algorithm>
#include <set>
#include <vector>

namespace cfml {

cfvariant *cf_structcopy(const cfvariant *st) {
    if (!st || st->m_type != cfvariant::Struct) {
        throw webstrada::exception("StructCopy: First argument must be a struct");
    }
    // StructCopy must return an independent deep copy: ordinary copy now shares
    // the payload (CF reference semantics).
    auto *ret = new cfvariant(st->deepCopy());
    return ret;
}

} // namespace cfml
