#pragma once

#include <webstrada/cf8.h>
#include <webstrada/string.h>
#include <webstrada/profiler_store.h>
#include <libxml/tree.h>
#include <chrono>
#include <deque>
#include <map>
#include <set>
#include <string>
#include <vector>


// shared thread-local state (global scope, matching the definitions in core_*.cpp)
struct UdfCallCtx {
    webstrada::cfvariant *localScope;
    webstrada::cfvariant *parentScope;
    std::set<webstrada::string> localNames;
    // Numeric <cfloop> indices are active local bindings. Keep the live value
    // here as well as in the local struct so lookup cannot observe a stale
    // copied struct cell while a loop is executing.
    std::map<webstrada::string, webstrada::cfvariant> loopIndices;
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

struct RequestProfiler {
    double onRequestStartTime = 0;
    double templateExecTime = 0;
    double onRequestEndTime = 0;

    int cfcMethodCount = 0;
    double cfcMethodTime = 0;

    int cfcInstantiateCount = 0;
    double cfcInstantiateTime = 0;

    int customTagCount = 0;
    double customTagTime = 0;

    int queryCount = 0;
    double queryTime = 0;

    int includeCount = 0;
    double includeTime = 0;

    void reset() {
        onRequestStartTime = templateExecTime = onRequestEndTime = 0;
        cfcMethodCount = 0;
        cfcMethodTime = 0;
        cfcInstantiateCount = 0;
        cfcInstantiateTime = 0;
        customTagCount = 0;
        customTagTime = 0;
        queryCount = 0;
        queryTime = 0;
        includeCount = 0;
        includeTime = 0;
    }
};
extern thread_local RequestProfiler g_reqProfiler;

struct CustomTagCallCtx {
    std::string tagName;                 // Base name uppercased, e.g. "OUTER" / "GD"
    std::string publicName;              // GetBaseTagList name, e.g. "CF_OUTER" / "CF_GD"
    std::string templateName;            // GetBaseTagData match name, e.g. "cf_outer" / "CF_GD"
    webstrada::cfvariant attributes;     // Struct of attributes passed to custom tag
    webstrada::cfvariant thisTag;        // Struct: executionMode, hasendtag, GeneratedContent
    webstrada::cfvariant variables;      // Custom tag's private variables scope
    webstrada::cfvariant *callerVariables = nullptr; // Pointer to caller's variables scope
    bool hasEndTag = false;
    bool loopRequested = false;   // cfexit method="loop" requested a body re-run
    bool skipBody = false;        // bare cfexit in start template: skip body + end tag
    bool contentChanged = false;  // end template assigned thisTag.generatedContent
};
// A deque: a nested custom tag captures the parent's `ctx.variables` address
// as its `callerVariables`, and push_back on a std::vector would move the
// parent element (invalidating that pointer). deque keeps element addresses
// stable across push_back.
extern thread_local std::deque<CustomTagCallCtx> g_customTagStack;

// CF's "base tag" execution stack (innermost first), mirroring GetBaseTagList's
// model of the currently-executing tag contexts. A <cfoutput> block pushes
// "CFOUTPUT" while it runs and a custom tag pushes its public name ("CF_<NAME>");
// GetBaseTagList walks it from the innermost entry. This is kept as a separate
// string stack (instead of marker entries inside g_customTagStack) so the custom
// tag context consumers are never exposed to pseudo-entries.
extern thread_local std::vector<std::string> g_baseTagStack;
// The variables scope of the custom-tag template currently executing. Custom
// tags may run inside a component method, but their unqualified names must
// resolve to the custom tag's private variables before the surrounding UDF.
extern thread_local webstrada::cfvariant *g_customTagExecutionVariables;

namespace cfml {
void cf_custom_tag_begin(const char *tagName, cfvariant *attrs, bool hasEndTag, cfvariant *callerVariables,
                         bool isModule, const char *templateNameHint);
void cf_custom_tag_end_mode(const string *generatedContent);
void cf_custom_tag_finish();
void cf_cfoutput_begin();
void cf_cfoutput_end();
bool cf_custom_tag_should_loop();
bool cf_custom_tag_should_skip_body();
void cf_custom_tag_mark_content_changed();
cfvariant *cf_custom_tag_module_path(const cfvariant *templateAttr, const cfvariant *nameAttr);
void cf_custom_tag_merge_attributecollection(cfvariant *attrs, const cfvariant *collection);
void cf_custom_tag_invoke(string *out, void *cgi, void *server, void *cookie, void *application,
                          void *session, void *url, void *form, void *variables,
                          cfvariant *tagPathVar, const char *tagPath, const char *tagName,
                          cfvariant *attrs, string *bodyContent, bool hasEndTag, bool isEndMode,
                          bool isModule, const char *templateNameHint);
}

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

// Resolves the target scope for an unqualified variable assignment, exactly
// like cfvariant_assign's unqualified branch: inside a UDF a var-declared
// (local) name writes to the local scope, a parameter name to the `arguments`
// scope, and everything else to the captured parent scope; outside a UDF the
// plain variables scope is used. Used by the cfloop index assignment so the
// loop variable lands in the scope an unqualified <cfset> would target (a
// `var`-declared loop index lives in `local`, not the component variables
// scope — was a WebStrada divergence from CF in CFC methods).
webstrada::cfvariant *udfAssignScope(webstrada::cfvariant *variables, const char *name);

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
    const webstrada::cfvariant **args, int arg_count,
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

// Request execution tracer helpers (core_stacktrace.cpp)
void trace_begin_request(std::chrono::steady_clock::time_point acceptTime = std::chrono::steady_clock::now());
void trace_record_event(const char *type, const char *path, const char *function, int line = 0);
std::vector<webstrada::TraceStep> trace_take_steps();
}
