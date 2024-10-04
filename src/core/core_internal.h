#pragma once

#include <webstrada/cf8.h>
#include <webstrada/string.h>
#include <libxml/tree.h>
#include <set>
#include <string>
#include <vector>


// shared thread-local state (global scope, matching the definitions in core_*.cpp)
struct UdfCallCtx {
    webstrada::cfvariant *localScope;
    webstrada::cfvariant *parentScope;
    std::set<webstrada::string> localNames;
    // Component-method context: when this call runs inside a component method
    // (or the component's construction body), `thisScope` is the component's
    // `this` scope and `component` the ComponentInstance. Used so unqualified
    // names fall through to the component's variables/this scopes and `this`
    // resolves, and internal method calls can find private methods.
    webstrada::cfvariant *thisScope = nullptr;
    webstrada::ComponentInstance *component = nullptr;
    webstrada::ComponentInfo *componentInfo = nullptr;
};
extern thread_local std::vector<UdfCallCtx> g_udfCtx;
extern thread_local std::string g_requestBody;

// Inside a plain (non-component) UDF body, CF's `variables` scope is the
// CALLING page's variables scope (the captured parent scope), not the
// function-local scope — `variables.foo = 1` inside such a function sets a page
// variable (was BUGS.md "UDF: variables.foo"). The UDF body compiles with its
// localScope as the `variables` argument, so `variables` (the argument) must be
// redirected to the parent scope when it aliases the current UDF's local scope.
// Component methods are unaffected (their `variables` is the instance scope).
webstrada::cfvariant *udfVariablesScope(webstrada::cfvariant *passedVariables);

// Returns the current UDF's `arguments` struct (the "ARGUMENTS" key of its
// local scope) or nullptr. CF keeps function arguments out of the `local`
// scope; unqualified names fall through to `arguments` after `local`.
webstrada::cfvariant *udfArgumentsScope(const webstrada::cfvariant *localScope);

// ---- promoted from cf8.cpp (split into core/). These were file-scope helpers
// defined at global scope with "using namespace webstrada; using namespace cfml;".
// (Global-scope ones live in ::; the cfml::-qualified ones are wrapped below.)

using namespace webstrada;
using namespace cfml;

bool isBareIdentifier(const webstrada::string &s);
bool isKnownFunctionName(const webstrada::string &name);
webstrada::string functionHandleText(const webstrada::string &name);
webstrada::cfvariant makeFunctionHandle(const webstrada::string &name);
webstrada::cfvariant evaluateExpr(webstrada::string &out, const webstrada::string &expr,
    void *cgi, void *server, void *cookie, void *application,
    void *session, void *url, void *form, void *variables,
    bool parseBinary = true);
webstrada::cfvariant invokeMemberMethod(
    webstrada::cfvariant &base, const webstrada::string &methodName,
    std::vector<webstrada::cfvariant> &args,
    webstrada::string &out, void *cgi, void *server, void *cookie, void *application,
    void *session, void *url, void *form, void *variables);
webstrada::cfvariant *resolveQueryMember(webstrada::cfvariant *query, const char *key);

// Component member access (core_component.cpp): returns a this-scope data
// member or a callable method-handle for an externally callable method, or
// nullptr when the member does not exist.
namespace webstrada {
cfvariant *componentMemberAccess(cfvariant *comp, const webstrada::string &key);
int findMethodInInfo(ComponentInfo *info, const std::string &upper, ComponentInfo *&owner);

// Descend a dotted member path from `base` (used by the scope lookups).
cfvariant *descendDottedPath(cfvariant *base,
                             const std::vector<webstrada::string> &parts,
                             size_t startIdx);
} // namespace webstrada

namespace cfml {
// A temporary Component variant for `this` (defined in core_component.cpp).
webstrada::cfvariant *componentThisValue(webstrada::ComponentInstance *inst);
}
webstrada::string scalarJavaTypeName(const webstrada::cfvariant *v);
void storeQueryColumnRef(webstrada::cfvariant &v);
webstrada::cfvariant scalarizeQueryColumn(const webstrada::cfvariant *v);
int64_t nowSeconds();
int compareVariants(const webstrada::cfvariant *a, const webstrada::cfvariant *b, const webstrada::string &op);

namespace cfml {
webstrada::cfvariant *lookupVarWritable(const char *name,
    void *cgi, void *server, void *cookie, void *application,
    void *session, void *url, void *form, void *variables);
// Component introspection (core_component.cpp).
webstrada::cfvariant *cf_getcomponentmetadata_impl(const webstrada::cfvariant *obj);
webstrada::cfvariant *cf_isinstanceof_impl(const webstrada::cfvariant *obj, const webstrada::cfvariant *typeName);
// XML helpers defined in core_xml.cpp
webstrada::cfvariant create_xml_node(xmlNodePtr node, bool caseSensitive);
webstrada::cfvariant create_xml_document(xmlDocPtr doc, bool caseSensitive);
std::string safe_to_std_string(const webstrada::string *s);
std::string safe_to_std_string(const webstrada::string &s);
std::string safe_to_std_string(const webstrada::cfvariant &v);
std::string serialize_xml_node(const webstrada::cfvariant &node);
}