/**
 * @file fn_structgetmetadata.cpp
 * @brief CFML structgetmetadata() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <algorithm>
#include <set>
#include <vector>

namespace cfml {

cfvariant *cf_structgetmetadata(const cfvariant *st) {
    if (!st || st->m_type != cfvariant::Struct) {
        throw webstrada::exception("StructGetMetadata: First argument must be a struct");
    }
    auto *ret = new cfvariant(cfvariant::Struct);
    // Our structs are all insertion-ordered and case-insensitive; CF reports
    // the inherent properties of a regular struct as unordered / case-insensitive.
    // The keys are lowercase in CF (verified vs CF 2021).
    ret->structSet("ordered", cfvariant("unordered"));
    ret->structSet("casesensistive", cfvariant("NO"));
    if (st->m_structData && st->m_structData->meta) {
        ret->structSet("keys", *st->m_structData->meta);
    }
    return ret;
}

} // namespace cfml
