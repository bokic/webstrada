/**
 * @file fn_extensions.cpp
 * @brief Compiler-extension function registry (the `__` prefix family).
 *
 * Names starting with `__` are reserved like C's `__` identifiers: they are
 * engine-provided extensions, never runtime-dispatched UDFs or shadowable
 * variables. The registry maps the upper-cased CFML name to its native
 * implementation (uniform ABI: const cfvariant **args + int argc).
 *
 * The JIT compiler resolves them as direct calls to cf___<name>(args, argc)
 * via the AddSymbol table (llvm_compiler.cpp); this registry serves the
 * #...# output-expression interpreter (core_interp.cpp) and the compile-time
 * validation of both paths.
 */

#include "common.h"

#include <webstrada/cf8.h>

#include <map>
#include <string>

namespace cfml {

namespace {

using ExtensionFn = cfvariant *(*)(const cfvariant **args, int argc);

const std::map<std::string, ExtensionFn> &extensionRegistry()
{
    static const std::map<std::string, ExtensionFn> table = {
        {"__CONFIGGET", cfml::cf___configget},
        {"__CONFIGSET", cfml::cf___configset},
        {"__DATASOURCETEST", cfml::cf___datasourcetest},
        {"__SERVERINFO", cfml::cf___serverinfo},
        {"__CONFIGRESET", cfml::cf___configreset},
        {"__CACHEINFO", cfml::cf___cacheinfo},
        {"__CACHEEVICT", cfml::cf___cacheevict},
        {"__CACHECLEAR", cfml::cf___cacheclear},
        {"__REQUESTTRACE", cfml::cf___requesttrace},
        {"__TRACECONTROL", cfml::cf___tracecontrol},
    };
    return table;
}

} // namespace

bool cf_is_extension_name(const char *upperName)
{
    if (!upperName) return false;
    return extensionRegistry().find(upperName) != extensionRegistry().end();
}

cfvariant *cf_extension_call(const char *upperName, const cfvariant **args, int argc)
{
    if (!upperName) return nullptr;
    auto it = extensionRegistry().find(upperName);
    if (it == extensionRegistry().end()) return nullptr;
    return it->second(args, argc);
}

} // namespace cfml
