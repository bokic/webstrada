/**
 * @file fn_structsetmetadata.cpp
 * @brief CFML structsetmetadata() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <algorithm>
#include <set>
#include <vector>

namespace cfml {

cfvariant *cf_structsetmetadata(cfvariant *st, const cfvariant *meta) {
    if (!st || st->m_type != cfvariant::Struct) {
        throw webstrada::exception("StructSetMetadata: First argument must be a struct");
    }
    if (!meta || meta->m_type != cfvariant::Struct) {
        throw webstrada::exception("StructSetMetadata: Second argument must be a struct");
    }
    if (st->m_structData) {
        delete st->m_structData->meta;
        st->m_structData->meta = new cfvariant(meta->deepCopy());
    }
    auto *ret = new cfvariant(cfvariant::Null);
    return ret;
}

} // namespace cfml
