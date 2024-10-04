/**
 * @file fn_invoke.cpp
 * @brief CFML invoke() built-in.
 *
 * invoke(object, methodName [, arguments]) invokes a CFC method or a
 * user-defined function. CF behavior (CFPage.invoke):
 *   - A null/empty `object` invokes the UDF named `methodName` in the current
 *     page context (invokeUDF = true).
 *   - A String `object` is a CFC path that is loaded and instantiated.
 *   - `arguments` is either an array (positional) or a struct (named); the
 *     result is passed to the method and returned.
 */

#include "common.h"

#include "../cftags/common.h"
#include "../core/core_internal.h"
#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>
#include <webstrada/exceptions.h>
#include <webstrada/string.h>
#include <string>
#include <vector>

namespace cfml {

using webstrada::cfvariant;
using webstrada::string;

cfvariant *cf_invoke(const cfvariant *object, const cfvariant *methodName, const cfvariant *arguments,
                     string &out, void *cgi, void *server, void *cookie, void *application,
                     void *session, void *url, void *form, void *variables)
{
    if (!methodName) throw webstrada::exception("invoke requires at least 2 arguments");

    std::string mname = safe_to_std_string(*methodName);
    bool invokeUDF = false;
    cfvariant *target = nullptr;
    cfvariant loaded;

    if (!object || object->m_type == cfvariant::Null) {
        invokeUDF = true;
    } else if (object->m_type == cfvariant::String) {
        std::string o = safe_to_std_string(*object);
        if (o.empty()) {
            invokeUDF = true;
        } else {
            ComponentInfo *info = cf_component_load(o.c_str());
            if (!info) {
                throw webstrada::exception("invoke",
                    webstrada::string(("The component " + o + " could not be found.").c_str()));
            }
            loaded = *cf_component_instantiate(info, static_cast<cfvariant*>(variables),
                                               &out, cgi, server, cookie, application, session, url, form);
            cf_component_info_release(info);
            target = &loaded;
        }
    } else if (object->m_type == cfvariant::Component) {
        target = const_cast<cfvariant*>(object);
    } else {
        throw webstrada::exception("invoke",
            "The object argument must be a component instance, a component path, or empty.");
    }

    // Build the argument list. A struct is passed as a named-argument marker
    // (args[0]) so the invocation reorders against the parameter names; an
    // array is passed positionally.
    std::vector<const cfvariant*> argPtrs;
    cfvariant marker(cfvariant::Null);
    if (arguments) {
        if (arguments->m_type == cfvariant::Struct && arguments->m_struct) {
            cfvariant named(cfvariant::Struct);
            for (const auto &kv : *arguments->m_struct) {
                named.structSet(kv.first, kv.second);
            }
            cfvariant *m = cf_named_args_marker(&named);
            cf_register_temp(m);
            marker = *m;
            argPtrs.push_back(&marker);
        } else if (arguments->m_type == cfvariant::Array && arguments->m_array) {
            for (const auto &a : *arguments->m_array) {
                argPtrs.push_back(&a);
            }
        } else {
            throw webstrada::exception("invoke",
                "The arguments argument must be an array or a struct.");
        }
    }

    if (invokeUDF) {
        // Resolve methodName as a UDF in the current context and invoke it.
        cfvariant *udfVal = lookupVarWritable(mname.c_str(), cgi, server, cookie, application, session, url, form, variables);
        if (udfVal && udfVal->m_type == cfvariant::Function && udfVal->m_udf && udfVal->m_udf->fn) {
            if (udfVal->m_udf->componentMethodIndex >= 0 && udfVal->m_udf->component) {
                return cf_component_method_handle_invoke(udfVal, argPtrs.data(), static_cast<int>(argPtrs.size()),
                                                         out, cgi, server, cookie, application, session, url, form);
            }
            return cf_udf_invoke(udfVal, argPtrs.data(), static_cast<int>(argPtrs.size()),
                                 out, cgi, server, cookie, application, session, url, form, variables);
        }
        throw webstrada::exception("invoke",
            webstrada::string(("The method " + mname + " was not found.").c_str()));
    }

    if (!target) {
        throw webstrada::exception("invoke",
            "The object argument could not be resolved.");
    }
    return cf_component_invoke(target, mname.c_str(), argPtrs.data(), static_cast<int>(argPtrs.size()),
                               out, cgi, server, cookie, application, session, url, form);
}

} // namespace cfml
