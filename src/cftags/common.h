#pragma once

#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>
#include <webstrada/exceptions.h>
#include <webstrada/string.h>
#include <webstrada/db.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <json-c/json.h>

#include <deque>
#include <vector>
#include <string>

namespace cfml {

// Shared state/helpers defined in common.cpp.

// <cfhttp> request context: cf_http_begin pushes a fresh builder; cf_http_param
// appends a parameter; cf_http_end pops it.
struct HttpParam {
    std::string type;       // header/url/formfield/cgi/cookie/body/xml/file
    std::string name;
    std::string value;
    std::string file;       // file type: absolute path to send
    std::string mimetype;   // file type: content-type
    bool encoded = true;
};

struct HttpRequestCtx {
    webstrada::cfvariant attrs;                // evaluated attribute struct
    std::vector<HttpParam> params;
};

extern thread_local std::vector<HttpRequestCtx*> g_httpCtxs;

// <cfloop query> scope stack (common.cpp).
extern thread_local std::vector<webstrada::cfvariant*> g_queryScopes;

// Discard buffer stack for <cfsilent> / <cfxml> / <cfquery>
webstrada::string *silent_buf_push();
void silent_buf_pop();
void silent_buf_clear();
bool silent_buf_contains(const webstrada::string *buf);

// The real (non-discard) output buffer under the current innermost silent
// buffer, for a <cfcontent> reset inside <cfsilent> to clear the whole page
// like CF. Returns nullptr when no silent buffer is active.
webstrada::string *silent_real_out();
// Records the real output buffer paired with the just-pushed silent buffer.
void silent_set_real_out(webstrada::string *realOut);

// Active <cfloop query> scopes
void query_scope_clear();
webstrada::cfvariant *query_scope_resolve_member(const std::vector<webstrada::string> &parts);

// Active <cftransaction> frame stack
struct TxFrame {
    std::string dsn;
    // Active connection (owned by the frame once a query has opened it; the
    // first <cfquery> inside the transaction opens it and calls begin()).
    webstrada::db::DBConnection *conn = nullptr;
    bool inTransaction = false;
    // Active named savepoints (TransactionSetSavePoint) in creation order.
    std::vector<std::string> savepoints;
};
extern thread_local std::vector<TxFrame> g_txStack;
TxFrame *transaction_get_active(const std::string &dsn);
void transaction_clear_all();

// <cfstoredproc> call context: cf_storedproc_begin pushes a fresh builder;
// each compiled <cfprocparam>/<cfprocresult> appends to it; cf_storedproc_end
// pops it and executes the call.
struct StoredProcParam {
    std::string type;        // in/out/inout (lowercase)
    std::string variable;    // out variable name ("" for in params)
    std::string value;
    std::string cfsqltype;   // CF_SQL_* (uppercased)
    int maxlength = -1;
    int scale = 0;
    bool isNull = false;
    std::string dbVarName;
};

struct StoredProcResultBinding {
    std::string name;
    int resultset = 1;
    long long maxrows = -1;
};

struct StoredProcCtx {
    std::vector<StoredProcParam> params;
    std::vector<StoredProcResultBinding> results;
};

extern thread_local std::vector<StoredProcCtx*> g_spCtxs;
void stored_proc_clear();

// <cfinvoke> call context: cf_cfinvoke_begin pushes a fresh builder; each
// compiled <cfinvokeargument> appends a named argument to it; cf_cfinvoke_end
// pops it and performs the invoke (component path / UDF).
struct InvokeArg {
    webstrada::string name;
    const webstrada::cfvariant *value = nullptr;
};

struct InvokeCtx {
    const webstrada::cfvariant *component = nullptr;         // component value / path
    const webstrada::cfvariant *method = nullptr;
    const webstrada::cfvariant *returnvariable = nullptr;
    const webstrada::cfvariant *argumentcollection = nullptr; // struct of named args
    std::vector<InvokeArg> args;                            // <cfinvokeargument> children
};

extern thread_local std::vector<InvokeCtx*> g_invokeCtxs;
void invoke_clear();

// <cfimport path="..."> registered component import paths (a dotted component
// path like "mylib.subcomp" or a wildcard "mylib.*"). Consulted by
// CreateObject/`new` as a fallback when the plain relative resolution misses.
extern thread_local std::vector<std::string> g_importPaths;
void import_paths_clear();

// Application.cfc this.mappings mappings: map of virtual prefix (e.g. "/org/mangoblog")
// to filesystem path (e.g. "/path/to/components").
extern thread_local std::map<std::string, std::string> g_appMappings;
void app_mappings_clear();
void app_mappings_set(const webstrada::cfvariant *mappingsVariant);
bool app_mappings_resolve(const std::string &path, std::string &resolved);

// Search implicit scopes flag
extern thread_local bool g_searchImplicitScopes;

// Per-request REQUEST scope (defined in tag_application.cpp, reset per request
// by scope_begin). Exposed so the scope lookups (lookupVarWritable/lookupVar/
// cfvariant_assign) can resolve explicit `request.foo` references.
extern thread_local webstrada::cfvariant g_requestScope;

// Internal helpers shared between cf8.cpp and tag implementations
webstrada::cfvariant *lookupVarWritable(const char *name,
    void *cgi, void *server, void *cookie, void *application,
    void *session, void *url, void *form, void *variables);

std::string safe_to_std_string(const webstrada::string *s);
std::string safe_to_std_string(const webstrada::string &s);
std::string safe_to_std_string(const webstrada::cfvariant &v);
bool cfmlBoolean(const webstrada::cfvariant *v, bool defaultValue);
void custom_tag_target_cache_clear();

// Response-string helpers shared between the tag runtimes (common.cpp).
webstrada::string stripCRLF(const webstrada::string &s);
webstrada::string encodeControlChars(const webstrada::string &s);
bool urlContainsCFTokens(const webstrada::string &url);
void responseSetHeader(cfml::response_state &r, const char *name,
                       const webstrada::string &value);

// Response charset/encoding helpers shared between tag_content / tag_header
// (common.cpp). responseCharsetCanonical throws CF's "Unsupported encoding"
// for unknown names; isUtf8Charset / parseContentType / readFileBytes /
// encodeBuffer are used by <cfcontent> / <cfheader>.
webstrada::string responseCharsetCanonical(const webstrada::string &charset);
bool isUtf8Charset(const webstrada::string &charset);
void parseContentType(const webstrada::string &type, webstrada::string &mime,
                      webstrada::string &charset);
void readFileBytes(const webstrada::string &path, std::vector<std::byte> &out);
std::vector<char> encodeBuffer(const webstrada::string &out,
                               const cfml::response_state &r);
// Write `out` (internal UTF-8) to the web engine in the response charset
// (binary/UTF-8: straight from the buffer with no copy; other charsets: via a
// reusable thread-local scratch). Used by response_send_remaining / response_flush.
void sendEncoded(const webstrada::string &out, const cfml::response_state &r);

// Response send helpers (common.cpp): write the status/headers/cookies block,
// and the CLI's flushed-output sink used by response_flush.
void sendHeader(const cfml::response_state &r);
extern thread_local webstrada::string g_cli_flushed;

std::string scope_json_serialize(const webstrada::cfvariant &data);
bool scope_json_deserialize(const std::string &text, webstrada::cfvariant &out);

webstrada::cfvariant create_xml_document(xmlDocPtr doc, bool caseSensitive);
webstrada::cfvariant create_xml_node(xmlNodePtr node, bool caseSensitive);
std::string serialize_xml_node(const webstrada::cfvariant &node);

webstrada::string makeCfToken();
void setSessionCookies(const webstrada::string &cfid, const webstrada::string &token);

// Custom-tag / base-tag execution stacks (tag_custom.cpp): clears any entries a
// previous request left behind (e.g. an abort mid-<cfoutput>).
void custom_tag_stack_clear();

void init_server_scope(webstrada::cfvariant &serverScope);

} // namespace cfml
