/**
 * @file fn_structappend.cpp
 * @brief CFML structappend() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/component.h>
#include <webstrada/exceptions.h>
#include <algorithm>
#include <set>
#include <vector>

namespace cfml {

cfvariant *cf_structappend(cfvariant *dest, const cfvariant *source, const cfvariant *overwriteFlag) {
    if (!dest || (dest->m_type != cfvariant::Struct && dest->m_type != cfvariant::Component)) {
        throw webstrada::exception("StructAppend: First argument must be a struct");
    }
    // `this` is exposed to CFML as a temporary Component wrapper. Mutations
    // must target the live component this-scope, otherwise StructAppend(this,
    // ...) succeeds but the appended members disappear after the call.
    if (dest->m_type == cfvariant::Component && dest->m_component && dest->m_component->thisScope) {
        if (std::getenv("WEBSTRADA_DEBUG_COMPONENTS")) {
            fprintf(stderr, "[WebStrada][DebugComponent] structappend componentDest=%p liveThisScope=%p liveType=%d liveKeys=%zu sourceType=%d sourceKeys=%zu\n",
                    static_cast<void*>(dest), static_cast<void*>(dest->m_component->thisScope),
                    static_cast<int>(dest->m_component->thisScope->m_type),
                    dest->m_component->thisScope->m_struct ? dest->m_component->thisScope->m_struct->size() : 0,
                    source ? static_cast<int>(source->m_type) : -1,
                    source && source->m_struct ? source->m_struct->size() : 0);
            fflush(stderr);
        }
        dest = dest->m_component->thisScope;
    }
    if (!source || source->m_type != cfvariant::Struct) {
        throw webstrada::exception("StructAppend: Second argument must be a struct");
    }
    bool overwrite = true;
    if (overwriteFlag && overwriteFlag->m_type != cfvariant::Null) overwrite = isTruthy(*overwriteFlag);
    for (const auto &key : structOrderedKeys(*source)) {
        const cfvariant &val = source->m_struct->at(key);
        if (overwrite || !dest->m_struct->contains(key)) {
            dest->structSet(key, val);
        }
    }
    auto *ret = new cfvariant(cfvariant::Boolean);
    ret->m_bool = true;
    return ret;
}

} // namespace cfml
