/**
 * @file fn_callstackget.cpp
 * @brief CFML CallStackGet() built-in.
 *
 * Returns a snapshot of the live CFML call stack (cfml::g_callStack): an array
 * of structs, element [1] being the innermost frame (where the call was made),
 * each with the keys Template (full pathname), LineNumber and Function
 * (uppercased function/method name, empty for a plain template page). The
 * casing matches CF 2025 (verified on the RDS host).
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>

namespace cfml {

cfvariant *cf_callstackget() {
    auto *arr = new cfvariant(cfvariant::Array);
    for (auto it = g_callStack.rbegin(); it != g_callStack.rend(); ++it) {
        auto *frame = new cfvariant(cfvariant::Struct);
        frame->structSet("Template", cfvariant(webstrada::string(it->path.c_str())));
        frame->structSet("LineNumber", cfvariant(it->line));
        frame->structSet("Function", cfvariant(webstrada::string(it->function.c_str())));
        arr->insert(*frame);
        delete frame;
    }
    return arr;
}

} // namespace cfml
