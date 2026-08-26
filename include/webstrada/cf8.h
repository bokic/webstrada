/**
 * @file cf8.h
 * @brief CFML built-in tag and utility function declarations.
 *
 * Provides core runtime helpers used by compiled CFML templates:
 * file I/O, variable inspection (cfdump), and output appending.
 */

#pragma once

#include "cfvariant.h"
#include "string.h"
#include "scope_store.h"
#include "exceptions.h"
#include <string>
#include <vector>

namespace webstrada {
struct ComponentInfo;
struct ComponentInstance;
}

// Temporary variants allocated by runtime helper functions are registered with
// the current function frame and cleaned up when the frame exits or unwinds.
namespace cfml {
    void cf_register_temp(webstrada::cfvariant *v);
    // Seeds the C rand() generator from the system entropy source (time, pid,
    // clock). Called once per process at each entry point (daemon, forked
    // workers, CLI, unit tests) so CreateUUID()/CreateGUID() and any other
    // rand()-based helper mint a distinct value per process (BUGS.md
    // "CreateUUID()/CreateGUID() return the same value on every run").
    void seed_rand();
}

#include "locale.h"

namespace cfml
{

using namespace webstrada;

// ---- CFML call stack (stacktrace) ----
// The live stack of executing templates/functions: outermost (the request
// template) first, innermost last. The JIT emits cf_stack_push at every
// template / UDF / component-method / component-construction entry,
// cf_stack_set_line before each statement, and cf_stack_pop on every exit
// path (normal return and exception unwinding). When an exception unwinds, the
// first landing pad snapshots this stack into the exception's m_stackTrace
// (see cf_stack_capture_on_exception).
extern thread_local std::vector<webstrada::StackLevel> g_callStack;

// JIT runtime helpers (registered with LLVM's DynamicLibrary). cf_stack_push
// pushes a {path, line=0, function} frame; `function` is the uppercased
// function/method name ("" for a plain template or component construction
// body).
void cf_stack_push(const char *path, const char *function);
void cf_stack_set_line(int line);
void cf_stack_pop();

// Captures the current call stack into the in-flight exception object pointed
// to by `exn` (a landing-pad exception pointer) unless it already has one.
// Called from every JIT landing pad (function boundaries and try/catch) before
// the frame is popped / dispatch runs, so the snapshot always includes the
// exact frame + line where the error occurred.
void cf_stack_capture_on_exception(void *exn);

// Builds a TAGCONTEXT array (element [1] = innermost frame) from a captured
// stack trace. The caller owns the returned variant (structSet copies it).
webstrada::cfvariant *cf_stack_tagcontext(const std::vector<webstrada::StackLevel> &st);

// Reverse of cf_stack_tagcontext: reads a TAGCONTEXT array (innermost-first)
// back into an outermost-first StackLevel vector (used by cf_eh_throw so a
// rethrow keeps the original stack trace).
std::vector<webstrada::StackLevel> cf_stack_tagcontext_to_levels(const webstrada::cfvariant *tags);

double getDoubleValue(cfvariant v);
int getIntValue(cfvariant v);
long long getLongIntValue(cfvariant v);
string queryColJavaTypeName(QueryData *qd, int colIdx, int rowIdx);
cfvariant coerceQueryCell(const string &type, const cfvariant &raw);
string queryColumnList(const cfvariant *query);
int queryRecordCount(const cfvariant *query);
bool isCfArray(const cfvariant *v);
void throwNotArrayError(const cfvariant *v);
void throwXmlNodeListUnsupported(const char *fn);
std::vector<webstrada::string> argumentsVisibleKeys(const cfvariant *arguments);
cfvariant callCallback(string &out, const cfvariant &callback, const std::vector<cfvariant> &args, void *cgi, void *server, void *cookie, void *application, void *session, void *url, void *form, void *variables);
bool cfvariantsEqual(const cfvariant &a, const cfvariant &b);
bool cfvariantsEqualNoCase(const cfvariant &a, const cfvariant &b);
int isTruthy(const webstrada::cfvariant &val);
bool isTrue(const webstrada::cfvariant &val);
string variantToString(const cfvariant &v);
std::vector<string> parseList(const string &listStr, const string &delim);
bool isQueryColumnRef(const cfvariant *v);
cfvariant queryColumnFirstCell(const cfvariant *v);
string formatDecimal(double val);
string formatDollar(double val);
void stringToBytes(const webstrada::string &s, const webstrada::string &encoding, std::vector<std::byte> &out);
// Same conversion writing into a plain char buffer (used by the output-send
// path to avoid an intermediate std::vector<std::byte>).
void stringToBytes(const webstrada::string &s, const webstrada::string &encoding, std::vector<char> &out);

enum FormatMode {
    ModeDate,
    ModeTime,
    ModeDateTime
};

double tmToDays(const struct tm &tm);
struct tm daysToTm(double days);
string formatDateTime(double days, const string &mask, FormatMode mode, const LocaleInfo *locale = nullptr);
bool parseDateTimeStr(const string &s, double &days);
// Locale-aware date/time parsing backing LSParseDateTime / LSIsDate. Implements
// the CFLocaleBase.ParseDateTime algorithm (Java SimpleDateFormat patterns from
// the locale's JDK COMPAT data + the phase-1/phase-2 formatter loops + the
// remainder split). Returns false when the string is not a valid locale date.
bool parseDateTimeLocale(const string &s, const LocaleInfo *locale, double &days);
// True when `s` matches CF's date-string shape (separators + numeric fields),
// regardless of component range; distinguishes CF's two invalid-date messages.
bool looksLikeDateString(const string &s);
bool getDaysFromVariant(const cfvariant *arg, double &days);
double getDaysOrThrow(const cfvariant *arg, const char *funcName);
bool isLeapYear(int year);
int getDaysInMonthVal(int year, int mon);
void normalizeTm(struct tm &tm);

webstrada::string bytesToText(const std::vector<std::byte> &bytes, const webstrada::string &encoding);
void urlDecodeString(const webstrada::string &s, std::vector<std::byte> &out, bool strict);

// Computed-double rendering shared between fn_dump.cpp (cfdump) and the
// interpreter's expression-to-string path in cf8.cpp.
std::string formatCfdumpFloat(double d);

// Shared by the crypto functions (fn_crypto.cpp) and cf8.cpp's URLEncodedFormat.
extern const char cryptoHexDigits[];

// Charset-name normalization shared between fn_misc.cpp (ToString) and cf8.cpp's
// CharsetDecode / CharsetEncode.
webstrada::string normalizeCharsetName(const webstrada::string &encoding);

/**
 * @brief Read the entire contents of a file into a string.
 *
 * Opens @p pathname for reading and reads all data in 1 KiB chunks
 * until EOF. If any @c read() call fails (returns -1), the partially
 * accumulated data is discarded and an empty string is returned.
 *
 * @param pathname  Null-terminated path to the file to read.
 * @return          The file contents on success, or an empty string
 *                  if the file could not be opened or a read error occurred.
 */
string readfile(const char *pathname);

/**
 * @brief Generate an HTML cfdump representation of a cfvariant value.
 *
 * Appends a self-contained HTML fragment — including an embedded
 * @c \<style\> block and a @c \<script\> block with collapse/expand
 * helpers — followed by a formatted table representing @p var.
 *
 * Supported variant types: @c Number, @c String, @c Struct.
 * All other types render as the string @c "Unknown".
 *
 * @param[out] out  String to append the generated HTML to.
 * @param[in]  var  The variant value to dump.
 *
 * @note The CSS and JavaScript are emitted on every call. Avoid calling
 *       this more than once per HTTP response to prevent duplicate style
 *       and script blocks.
 */
void cfdump(string &out, const cfvariant &var);

/**
 * @brief Append raw text to the response output buffer.
 *
 * Used by compiled CFML templates to write literal output. Does nothing
 * if @p size is zero.
 *
 * @param[out] out   Output buffer to append to.
 * @param[in]  text  Pointer to the text data to append.
 * @param[in]  size  Number of bytes to append from @p text.
 */
void cfwriteoutput(string &out, const char *text, size_t size);
void cfoutputvariant(string &out, const cfvariant *value);

// Whitespace management helper used by compiled templates: appends a single
// space to `out` unless the output buffer is empty or already ends in
// whitespace. This is the runtime half of ColdFusion's whitespace management;
// the compile-time half decides when to call it.
void cf_whitespace_space(string &out);

// <cfsilent> runtime: pushes a fresh discard buffer on a per-thread stack and
// returns it. The compiled <cfsilent> body writes to it instead of the real
// response buffer, so its output is suppressed. cf_silent_end() pops it; the
// stack is reset per request (scope_begin) so an exception unwinding past
// cf_silent_end() cannot leak a buffer into the next request.
string *cf_silent_begin(string *realOut);
void cf_silent_end();

// ---- <cfsetting> / <cfprocessingdirective> / <cfhtmlhead> / <cfcookie> /
//      <cfsavecontent> runtime (tag_weboutput.cpp) ----

// Runtime output-mode flag backing <cfsetting enablecfoutputonly="true|false">.
// While enabled, plain text written outside <cfoutput> (including the
// whitespace-collapse space) is suppressed, exactly like CF's
// CFOutput.enablecfoutputonly; the codegen routes non-<cfoutput> text writes
// through cf_write_output_gated / cf_whitespace_space_gated which check it.
// Reset per request in scope_begin.
bool cf_cfoutputonly_enabled();
void cf_cfoutputonly_set(bool enabled);

// Gated text/space writers: append `text` (or a single managed space) to `out`
// unless <cfsetting enablecfoutputonly> is currently on. The JIT calls these
// for every plain-text write compiled OUTSIDE a <cfoutput> body, so
// enablecfoutputonly suppresses exactly the content outside cfoutput tags.
void cf_write_output_gated(string &out, const char *text, size_t size);
void cf_whitespace_space_gated(string &out);

// <cfsetting> runtime. enablecfoutputonly/showdebugoutput/requesttimeout may be
// null when absent. enablecfoutputonly flips the cfoutputonly mode;
// showdebugoutput is accepted but this engine has no debug output section;
// requesttimeout is accepted but this engine has no request watchdog.
void cf_setting(const cfvariant *enablecfoutputonly, const cfvariant *showdebugoutput,
                const cfvariant *requesttimeout);

// <cfhtmlhead> runtime: appends `text` to the response head content, which CF
// writes before the body output no matter where the tag appeared in the page.
void cf_htmlhead_append(const cfvariant *text);

// <cfcookie> runtime. Each attribute may be null when absent (name is required
// by the compiler). Builds the Set-Cookie header (matching CF's CookieScope:
// Tomcat's cookie serializer for plain non-empty cookies, CF's own
// createCookieHeader when SameSite is set or the value is empty), records the
// cookie in the current request's COOKIE scope (the raw, unencoded value) and
// validates the name/expires like CF.
void cf_cookie_tag(cfvariant *cookie, const cfvariant *name, const cfvariant *value,
                   const cfvariant *expires, const cfvariant *secure, const cfvariant *path,
                   const cfvariant *domain, const cfvariant *httponly,
                   const cfvariant *encodevalue, const cfvariant *preservecase,
                   const cfvariant *samesite);

// <cfsavecontent> runtime: cf_savecontent_begin pushes a fresh capture buffer
// (the compiled body writes to it) and returns it; cf_savecontent_end pops it
// and assigns the captured text to the `variable` name (a dotted/scoped/dynamic
// name resolved like an assignment, matching CF's SaveContentTag.setVar). The
// buffer stack is reset per request (scope_begin) so an exception unwinding
// past cf_savecontent_end cannot leak one. The exception path (the compiled
// body is wrapped in a catch-all that calls cf_savecontent_end_assign first)
// still stores the partial content, exactly like CF's doCatch.
string *cf_savecontent_begin(string *realOut);
void cf_savecontent_validate(const cfvariant *varName);
void cf_savecontent_end_assign(string *captured, void *cgi, void *server, void *cookie,
                               void *application, void *session, void *url, void *form,
                               void *variables, const cfvariant *varName);
void cf_savecontent_end(string *captured, void *cgi, void *server, void *cookie,
                        void *application, void *session, void *url, void *form,
                        void *variables, const cfvariant *varName);

// ---- Custom tag runtime (tag_custom.cpp) ----
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

// <cfxml> runtime: cf_xml_begin pushes a fresh capture buffer (the compiled
// <cfxml> body writes to it) and returns it; cf_xml_end pops it, trims the
// captured text like CF's `varStr.trim()`, parses it as an XML document
// (respecting the tag's caseSensitive attribute) and assigns the result to the
// `variable` attribute (a dotted/scoped/dynamic name is resolved like an
// assignment). A parse failure throws CF's "An error occurred while Parsing an
// XML document." Expression error. The buffer stack is reset per request
// (scope_begin) so an exception unwinding past cf_xml_end cannot leak one.
string *cf_xml_begin();
void cf_xml_end(string *out, void *cgi, void *server, void *cookie, void *application,
                void *session, void *url, void *form, void *variables,
                const cfvariant *varName, const cfvariant *caseSensitive);

// <cfquery> runtime: cf_query_begin pushes a fresh capture buffer (the compiled
// <cfquery> body, with its #...# expressions evaluated, writes to it) and
// returns it; cf_query_end pops it, executes the captured SQL against the
// datasource's SQLite database (DNS_<name>.sqlite next to the executable) and
// stores the result query in the `name` attribute variable plus the `result`
// metadata struct. The buffer stack is reset per request (scope_begin) so an
// exception unwinding past cf_query_end cannot leak one.
string *cf_query_begin();
cfvariant *cf_query_end(string *sqlCapture, const cfvariant *attrs,
                        void *cgi, void *server, void *cookie, void *application,
                        void *session, void *url, void *form, void *variables);

// <cfqueryparam> runtime: validates/coerces a query parameter (value,
// cfsqltype, maxlength, scale, null, list, separator) and appends its formatted
// SQL literal to the `out` capture buffer, so a <cfquery> body containing
// <cfqueryparam> executes with the parameter substituted inline (this engine
// has no JDBC bind layer; queryExecute substitutes inline literals the same
// way). Type coercion reproduces CF's QueryParamTag: integer types truncate
// decimals and throw "Invalid data X for CFSQLTYPE CF_SQL_INTEGER.", numeric
// types throw "... CF_SQL_NUMERIC.", maxlength throws "Invalid data value X
// exceeds maxlength setting N.", date types throw "X is an invalid date or time
// string.", and list=true expands the value by separator.
void cf_queryparam(void *out, const cfvariant *value, const cfvariant *cfsqltype,
                   const cfvariant *maxlength, const cfvariant *scale,
                   const cfvariant *null, const cfvariant *list,
                   const cfvariant *separator);

// <cfinsert> runtime: builds an INSERT from the FORM scope (or the `formfields`
// attribute's comma list) for the given datasource/tablename and executes it.
// A form field that is not a column of the table throws CF's "The X fieldname
// cannot be found in the Y table."; values are coerced to the column's type.
// (Named cf_insert_tag to avoid colliding with the Insert() built-in.)
void cf_insert_tag(const cfvariant *attrs,
               void *cgi, void *server, void *cookie, void *application,
               void *session, void *url, void *form, void *variables);

// <cfupdate> runtime: builds an UPDATE from the FORM scope (or formfields) keyed
// on the table's primary key column(s). A missing primary-key field throws CF's
// "Primary key field X, not found. The field X, was not found in the form
// input. This field is required to do an update because it is part of the
// primary key for the Y table."; unknown form fields throw the same fieldname
// error as <cfinsert>.
void cf_update(const cfvariant *attrs,
               void *cgi, void *server, void *cookie, void *application,
               void *session, void *url, void *form, void *variables);

// <cfdbinfo> runtime: retrieves datasource metadata (type tables/columns/
// version/procedures/foreignkeys/index/dbnames) and stores the result query in
// the `name` attribute variable. Attribute validation (missing name/table,
// invalid type) reproduces CF's CFDBINFO messages.
void cf_dbinfo(const cfvariant *attrs,
               void *cgi, void *server, void *cookie, void *application,
               void *session, void *url, void *form, void *variables);

// <cfstoredproc> / <cfprocparam> / <cfprocresult> runtime. cf_storedproc_begin
// pushes a per-thread call context (and a discard buffer for the tag body);
// each compiled <cfprocparam>/<cfprocresult> calls cf_proc_param /
// cf_proc_result to append to it; cf_storedproc_end pops the context, executes
// the procedure (CALL on MySQL/PostgreSQL; SQLite throws) and assigns the
// result sets, the out/inout parameter values and the statusCode/executionTime
// variables. The stack is reset per request (scope_begin) so an exception
// unwinding past cf_storedproc_end cannot leak a context.
string *cf_storedproc_begin();
void cf_proc_param(const cfvariant *type, const cfvariant *variable, const cfvariant *value,
                   const cfvariant *cfsqltype, const cfvariant *maxlength,
                   const cfvariant *scale, const cfvariant *isNull,
                   const cfvariant *dbvarname);
void cf_proc_result(const cfvariant *name, const cfvariant *resultset, const cfvariant *maxrows);
void cf_storedproc_end(const cfvariant *attrs,
                       void *cgi, void *server, void *cookie, void *application,
                       void *session, void *url, void *form, void *variables);

// <cfdirectory> runtime: list (builds the NAME/SIZE/TYPE/DATELASTMODIFIED/
// ATTRIBUTES/MODE/DIRECTORY/LINK query into the `name` variable) / create /
// delete / rename / copy. Attribute validation (unknown attributes, invalid
// ACTION value) is compile-time in the codegen for static literals; the runtime
// throws CF's catchable Application errors.
void cf_directory_tag(const cfvariant *attrs,
                      void *cgi, void *server, void *cookie, void *application,
                      void *session, void *url, void *form, void *variables);

// <cffile> runtime: read/readBinary/write/append/copy/move/rename/delete/upload/
// uploadall. `bodyContent` is the captured tag body for write/append (null
// otherwise). After every action the (empty) `cffile` struct is created like CF.
void cf_file_tag(const cfvariant *attrs,
                 void *cgi, void *server, void *cookie, void *application,
                 void *session, void *url, void *form, void *variables,
                 webstrada::string *bodyContent);

// <cfexecute> runtime: runs the `name` executable with the (tokenized) args,
// capturing stdout to the page, the `variable` attribute or the `outputfile`,
// and stderr to the `errorVariable`/`errorFile` (defaulting to the page is NOT
// done for stderr — it is dropped unless a destination is given). The timeout
// attribute (seconds) limits how long CF waits for the process output; when
// absent (or 0) CF does not wait at all and the output is read immediately
// (ProcessExecutor's no-blocking race, verified on CF 2025). Exec failure and
// timeout throw CF's catchable Application errors.
void cf_execute_tag(webstrada::string *out, const cfvariant *attrs,
                    void *cgi, void *server, void *cookie, void *application,
                    void *session, void *url, void *form, void *variables);

// <cfwddx> runtime: action cfml2wddx serializes the input to a WDDX packet,
// wddx2cfml deserializes a WDDX packet to CFML, cfml2js/wddx2js render
// JavaScript assignments (StringFormatter.scriptFormat). The result is stored
// in the `output` attribute variable (when given) or written to the page.
void cf_wddx_tag(webstrada::string *out, const cfvariant *attrs,
                 void *cgi, void *server, void *cookie, void *application,
                 void *session, void *url, void *form, void *variables);

// <cffeed> runtime: action read parses an RSS/Atom feed (from a URL or an XML
// file) into the feed struct (`name`), the metadata struct (`properties`), the
// entries query (`query`) and/or the raw XML (`xmlvar`); action create renders
// an RSS 2.0 / Atom 1.0 feed from the `name` struct or `query`+`properties`
// (+`columnmap`) into `xmlvar`/`outputfile`.
void cf_feed_tag(const cfvariant *attrs,
                 void *cgi, void *server, void *cookie, void *application,
                 void *session, void *url, void *form, void *variables);

// <cfftp> / <cfschedule> runtimes: NOT implemented — these only log the call
// (tag name + evaluated attributes) to the engine log on stderr and perform
// no FTP/scheduling work. Templates using the tags compile and run instead of
// failing with "Tag cfftp/cfschedule is not implemented".
void cf_ftp_tag(const cfvariant *attrs);
void cf_schedule_tag(const cfvariant *attrs);

// <cfzip> / <cfzipparam> runtime: cf_zip_begin pushes a per-thread context and
// a discard buffer for the tag body; each compiled <cfzipparam> calls
// cf_zip_param to append to it; cf_zip_end pops the context and performs the
// zip/unzip/list/read/readBinary/delete action.
void cf_zip_begin();
void cf_zip_param(const cfvariant *source, const cfvariant *content,
                  const cfvariant *entrypath, const cfvariant *filter,
                  const cfvariant *prefix, const cfvariant *recurse,
                  const cfvariant *charset, const cfvariant *encryptionalgorithm,
                  const cfvariant *password);
void cf_zip_end(const cfvariant *attrs,
                void *cgi, void *server, void *cookie, void *application,
                void *session, void *url, void *form, void *variables);
void zip_ctx_clear();

// <cfloop query> runtime: cf_query_scope_push registers the query so
// unqualified names resolve against its current row (columns + currentrow/
// recordcount/columnlist) and returns the cursor's previous row; the loop
// advances the cursor with cf_query_set_row and restores it (then calls
// cf_query_scope_pop) when done. cf_query_rowcount returns the number of rows,
// cf_query_group_next skips to the first row of the next group (used by the
// `group` attribute).
long long cf_query_rowcount(const cfvariant *query);
void cf_query_set_row(const cfvariant *query, long long row);
cfvariant *cf_query_resolve(const cfvariant *value,
                            void *cgi, void *server, void *cookie, void *application,
                            void *session, void *url, void *form, void *variables);
long long cf_query_scope_push(const cfvariant *query);
void cf_query_scope_pop();
long long cf_query_group_next(const cfvariant *query, const cfvariant *groupCol,
                              long long currentRow, long long endRow,
                              const cfvariant *caseSensitive);

// <cfhttp> / <cfhttpparam> runtime. cf_http_begin pushes a per-thread HTTP
// request builder holding the (evaluated) tag attributes; each compiled
// <cfhttpparam> calls cf_http_param to append a parameter; cf_http_end pops
// the builder, performs the request with libcurl, writes the body to the
// `path`/`file` when given, and stores the result struct (statusCode /
// fileContent / responseHeader / errorDetail / mimeType / text / charset /
// header) into the `result` attribute variable (default "cfhttp"). The stack
// is reset per request (scope_begin) so an exception unwinding past
// cf_http_end cannot leak a builder.
void cf_http_begin(const cfvariant *attrs);
void cf_http_param(const cfvariant *type, const cfvariant *name, const cfvariant *value,
                   const cfvariant *file, const cfvariant *encoded, const cfvariant *mimetype);
void cf_http_end(void *cgi, void *server, void *cookie, void *application,
                 void *session, void *url, void *form, void *variables);

// <cftransaction> runtime. cf_transaction_begin pushes a per-thread
// transaction frame (the datasource's SQLite connection is opened lazily by the
// first <cfquery> inside, which then runs on the shared connection); every
// <cfquery> inside joins the transaction. cf_transaction_end commits (or rolls
// back when an exception unwound the body) and pops the frame. The frame stack
// is reset per request (scope_begin) so an exception unwinding past
// cf_transaction_end cannot leak a connection.
void cf_transaction_begin(const cfvariant *attrs);
void cf_transaction_commit();
void cf_transaction_rollback();
void cf_transaction_setsavepoint(const std::string &name);
bool cf_transaction_rollback_to(const std::string &name);

// <cfsetlocale>/<cfgetlocale> and the LS* functions use a per-request current
// locale (a pointer into the locale table in src/locale_table.inc). locale_reset
// is called at request start (scope_begin); it is also useful to tests.
void locale_reset();

// ---- HTTP response output state (<cfcontent> / <cfflush>) ----

// Byte-writing callback used to send output to the web engine. The daemon
// worker installs a FastCGI writer; the CLI leaves it unset (flushes then only
// mark the response committed so post-flush charset changes are ignored,
// exactly like ColdFusion).
typedef void (*response_write_fn)(const char *data, size_t len);
void response_set_write_fn(response_write_fn fn);

// Per-request state that drives the output character encoding and MIME type.
// The daemon worker binds its FastCGI output stream before running a template;
// the CLI leaves `stream` null. Output is buffered in UTF-8 and encoded to
// `charset` only when it is sent to the web engine, so changing the charset is
// free until the first byte is committed.
struct response_state {
    string contentType = "text/html";
    string charset = "UTF-8";
    bool committed = false;  // first byte written to the web engine
    bool binary = false;     // raw bytes (cfcontent file/variable); skip encoding
    void *stream = nullptr;  // non-null when a web engine is attached

    // Content-Language header value ("en-US", ...) set by <cfsetlocale>;
    // emitted with the response headers when non-empty.
    string contentLanguage;

    // Pending Set-Cookie header bodies ("Name=value; Path=/..."), emitted with
    // the Content-Type header when the response is first committed. Used by the
    // session machinery to hand out CFID/CFTOKEN cookies.
    std::vector<std::string> cookies;

    // HTTP status code set by <cfheader statuscode> / <cflocation statusCode>.
    // Emitted as a CGI "Status:" line (the FastCGI convention for a non-200
    // status) when it differs from the default 200.
    int statusCode = 200;

    // Custom response headers added by <cfheader name=... value=...> plus the
    // <cflocation> Location/Cache-Control/Pragma headers. Each entry is
    // (name, value) with the name as CF passes it; emitted with the
    // Content-Type header when the response is first committed.
    std::vector<std::pair<std::string, std::string>> headers;

    // Content that CF places in the response <head> (e.g. the inline <script>
    // from ajaxOnLoad). CF writes head content before the body output no
    // matter where it was produced in the page; this engine accumulates it
    // here and response_encode_all() prepends it to the body output.
    string headContent;
};

// Current request's response state (thread-local).
response_state &response();

// Reset the response state to defaults for a new request. The output charset
// is taken from webstrada::config::defaultOutputCharset (UTF-8 unless changed).
void response_begin();

// Bind the web-engine output stream for the current request (null in the CLI).
void response_set_stream(void *stream);

// Queue a Set-Cookie header body (e.g. "CFID=123; Path=/"). The header is
// written together with the Content-Type header when the response is first
// committed (flushed or sent at request end). If the response is already
// committed the cookie is dropped, matching ColdFusion's "can't set cookies
// after output" rule.
void response_add_cookie(const char *name, const char *value, const char *path);

// Whether the first byte has already been written to the web engine.
int response_committed();

const char *response_charset_name();
const char *response_content_type_name();
int response_is_binary();

// <cfcontent> runtime. Each argument may be null when the attribute is absent.
// `type` is the raw MIME type string ("text/html; charset=ISO-8859-1"); `file`
// is a path whose contents replace the page output; `variable` a cfvariant
// (binary or string) whose contents replace the page output; `reset` /
// `deletefile` are CFML booleans. Once the response is committed, the type and
// charset can no longer be changed (silently ignored, matching ColdFusion).
void response_apply_cfcontent(string *out, const cfvariant *type, const cfvariant *reset,
                              const cfvariant *file, const cfvariant *variable,
                              const cfvariant *deletefile);

// <cfflush> runtime: flush the buffered output to the web engine. Does nothing
// when the buffer is empty (no byte is sent, so the response stays uncommitted).
void response_flush(string *out);

// <cfheader> runtime. Applies the name/value/charset/statuscode attributes to
// the response state. A `name` header is queued (its value re-encoded with
// `charset` when given); name="content-type" routes through the response MIME
// type (parsing any "; charset=" like <cfcontent>, which also changes the
// output charset), and `statuscode` sets the HTTP status. Throws CF's "Failed
// to add HTML header" error when the response is already committed. Any of the
// pointers may be null when the attribute is absent.
void response_add_header(const cfvariant *name, const cfvariant *value,
                         const cfvariant *charset, const cfvariant *statusCode);

// <cflocation> runtime. Sets the redirect status (default 302, validated to
// 300-307 like CF) plus the Location/Cache-Control/Pragma headers, appends the
// CFID/CFTOKEN session token to the URL when `addToken` is true (default) and a
// session was minted this request, then aborts the current page by throwing
// abort_exception. Headers/status are silently dropped when the response was
// already committed (matching ColdFusion, where only the abort happens).
[[noreturn]] void response_redirect(const cfvariant *url, const cfvariant *addToken,
                                    const cfvariant *statusCode);

// Send the buffered output to the bound web-engine stream, writing the
// Content-Type header first if it has not been committed yet. Used by the
// worker after the template finishes.
void response_send_remaining(string *out);

// Encode a UTF-8 output buffer into the current response charset (raw bytes
// when the response is in binary mode). Used by the CLI and the worker.
std::vector<char> response_encode(const string &out);

// Encode the CLI's flushed-output sink plus the current buffer (see the
// response_flush note about the CLI). Used by the CLI before writing stdout.
std::vector<char> response_encode_all(const string &out);

// ---- <cfcache> runtime ----

// <cfcache> start-tag runtime. `hasEndTag` is 1 for the body form
// (`<cfcache>...</cfcache>`), 0 for the self-closing form. Returns a Boolean
// cfvariant: true when the tag found cached content and wrote it to `out`
// (the compiled code must then skip the body, or skip the whole page for the
// self-closing form); false to continue normal processing. Attribute pointers
// may be null when the attribute is absent. The runtime performs the tag's
// catchable validations (directory/protocol/dependson, key+region, metadata
// without an end tag, self-closing inside an included template, ...) and the
// action dispatch (template/fragment cache, clientcache headers, flush, put,
// get); the whole-page miss registers a pending store that the request
// harness finalizes with cf_cache_store_page after the page completes.
cfvariant *cf_cache_tag_begin(
    string *out, void *cgi, void *server, void *cookie, void *application,
    void *session, void *url, void *form, void *variables,
    const cfvariant *action, const cfvariant *directory,
    const cfvariant *timespan, const cfvariant *idletime,
    const cfvariant *expireurl,
    const cfvariant *username, const cfvariant *password,
    const cfvariant *protocol, const cfvariant *port,
    const cfvariant *id, const cfvariant *key, const cfvariant *region,
    const cfvariant *dependson, const cfvariant *usecache,
    const cfvariant *stripwhitespace, const cfvariant *value,
    const cfvariant *name, const cfvariant *metadata,
    const cfvariant *throwonerror, const cfvariant *usequerystring,
    int hasEndTag, int lineNo);

// <cfcache> end-tag runtime: store the body output (captured between begin and
// end) into the template cache, and (when the tag declared a `metadata`
// variable) set it to the stored entry's metadata. Called by the compiled end
// tag on the miss path only (the hit path skips the body entirely).
void cf_cache_tag_end(string *out, void *variables);

// Request-harness hook called after a page completes: if a self-closing
// <cfcache> (cache/servercache/optimal) missed and registered a pending whole
// page store, serialize the final response (status, type, headers, cookies,
// body) into the template cache region. No-op otherwise.
void cf_cache_store_page(string *out);

// Reset the <cfcache> per-request thread-local state (pending whole-page store
// and fragment captures). The request harness calls response_begin at the start
// of each request; this also clears any state left over from a request that
// ended abnormally.
void cf_cache_reset();

// ---- <cfinclude> runtime ----

// Call shape of a compiled template (mirrors webstrada::template_fn without the
// std::function wrapper). Used by the <cfinclude> loader so cf8.cpp stays free
// of the LLVM/compiler headers.
typedef void (*include_template_fn)(
    string *out, void *cgi, void *server, void *cookie, void *application,
    void *session, void *url, void *form, void *variables);

// Per-request context for <cfinclude>. The request harness (daemon worker /
// CLI) installs one via include_begin()/include_end() so a compiled <cfinclude>
// can resolve its target path relative to the template that is currently
// executing and run other templates from the shared compiled-template cache.
struct IncludeRuntime {
    // Absolute path of the template currently executing. Relative includes
    // resolve against its directory; cf_include swaps it to the included
    // template's path while that template runs, so nested relative includes
    // resolve against the innermost template.
    std::string currentPath;
    // Web root; includes whose path starts with '/' resolve against it.
    std::string webRoot;
    // Loads (compiling if needed) the template at an absolute `path`. Returns
    // nullptr when the file does not exist.
    include_template_fn (*loader)(const char *path, void *opaque);
    void *loaderOpaque = nullptr;
    // Absolute paths already included in this request (runonce tracking).
    std::vector<std::string> runOnceIncluded;

    // Depth of nested <cfinclude> invocations for the currently executing
    // template. Incremented while an included template runs; a cfcache tag
    // with no end tag is rejected when this is non-zero (CF: "CFCache must
    // have an end tag if used inside an included template.").
    int includeDepth = 0;

    // Caller-local scope for the template currently executing through
    // <cfinclude>.  This is separate from `variables`: inside a component
    // method, `variables` remains the component instance scope while an
    // included template must still resolve unqualified names from the caller's
    // local scope.
    webstrada::cfvariant *includeLocalScope = nullptr;

    // Loads (compiling if needed) the ColdFusion component at an absolute
    // `path`, returning a retained ComponentInfo (or nullptr when the file
    // does not exist). Null when component loading is unavailable; the runtime
    // then throws "Components are not available in this context."
    webstrada::ComponentInfo *(*componentLoader)(const char *path, void *opaque);
    void *componentLoaderOpaque = nullptr;
};

// Install/clear the per-request include context. include_end clears the
// thread-local pointer; the harness must call it before the next request.
void include_begin(IncludeRuntime *rt);
void include_end();

// Returns the currently installed include context (null when none).
IncludeRuntime *include_context();

// <cfinclude> runtime helper called by compiled templates. `templatePath` is
// the evaluated template attribute (a cfvariant holding the string),
// `runonce` the runonce flag value (evaluated at runtime for truthiness).
// Throws when the target template cannot be resolved.
void cf_include(string *out, void *cgi, void *server, void *cookie, void *application,
                void *session, void *url, void *form, void *variables,
                const cfvariant *localScope, const cfvariant *templatePath,
                const cfvariant *runonce);

// ---- <cferror> runtime ----

// Reset the per-request <cferror> registry: clears every registered exception
// handler plus the request/validation handlers and the once-per-request
// dispatch marker (CF's FusionContext fields + the ExceptionFilter "biscuit").
// The request harness calls it at the start of each request.
void cferror_reset();

// <cferror> runtime invoked by compiled templates. The pointers are the
// evaluated attribute values (null when the attribute is absent; `exception`
// defaults to "any"). Mirrors CF 2025's ErrorTag.doStartTag: the template
// attribute is resolved against the page directory / web root and must exist
// (throwing "Error attempting to resolve the template {x}." / "The template
// could not be found.", type MissingInclude), then the type attribute
// dispatches to the exception handler registry, the request handler or the
// validation handler. An unrecognized type value throws CF's
// InvalidTagAttributeException message ("The value of the attribute type,
// which is currently {x}, is invalid.", type Application). Attribute
// validation (unknown / required attributes) is done at compile time.
void cf_cferror_register(string *out, void *cgi, void *server, void *cookie, void *application,
                         void *session, void *url, void *form, void *variables,
                         const cfvariant *type, const cfvariant *templateAttr,
                         const cfvariant *mailto, const cfvariant *exception);

// <cferror> exception dispatch, mirroring CF 2025's ExceptionFilter (minus the
// admin site-wide page): runs the closest matching exception handler; when its
// template throws, falls through to the request-type handler with the NEW
// exception (exception handlers are not re-run). Returns nonzero when a
// handler produced output — the caller must not render its own error page.
// Aborts/exits raised by a handler template are swallowed (handled). A missing
// template exception (m_missingTemplate) never reaches the handlers (CF's
// TemplateNotFoundException is not a MissingIncludeException); the
// once-per-request marker makes only the first exception dispatch per request.
// `requestPath` is the request's web path (the error.template value).
int cf_cferror_handle(const webstrada::exception *ex, string *out, void *cgi, void *server,
                      void *cookie, void *application, void *session, void *url, void *form,
                      void *variables, const char *requestPath);

// ---- APPLICATION / SESSION scope persistence (SQLite-backed) ----

// Per-request context for the SQLite-backed APPLICATION and SESSION scopes.
// The worker fills it at the start of each request (scope_begin) and the
// <cfapplication> JIT helpers + Session*/ApplicationStop functions consult it.
// scope_end() serializes the enabled scopes back into the store, so it must be
// called after the template finishes (RAII in the worker).
struct ScopeContext {
    ScopeStore *store = nullptr;
    cfvariant *application = nullptr;  // points at the worker's request slot
    cfvariant *session = nullptr;      // points at the worker's request slot
    std::string appName;               // "" when <cfapplication> has no name
    std::string sessionId;             // "CFID:CFTOKEN"; "" when no session
    double appTimeoutSeconds = 0;      // effective timeout (0 = never purge)
    double sessionTimeoutSeconds = 0;
    bool sessionManagement = false;
    bool applicationEnabled = false;   // <cfapplication> ran with a store
    bool sessionEnabled = false;
    bool sessionDirty = false;         // session data changed in this request
    bool appDirty = false;             // application data changed in this request
    bool sessionNewlyCreated = false;  // a fresh session was minted this request
    int64_t sessionStartTime = 0;      // unix epoch of session creation (0=unknown)
    // <cfapplication loginstorage="session">: the CFAUTHORIZATION login key is
    // stored in the session scope instead of a cookie (CF's
    // ApplicationScope.setStoreloginCredentialInSession). Default: cookie.
    bool loginStorageIsSession = false;
};

// Set up the request-scope context. `store` may be null (CLI) — the enable
// helpers then throw a "not available in this context" error.
void scope_begin(ScopeStore *store, cfvariant *application, cfvariant *session);

// Persist any enabled/dirty scope back into the store and clear the context.
// Safe to call when scope_begin was never invoked.
void scope_end();

ScopeContext &scope_context();

// <cfapplication> runtime. The compiled tag passes the request's application,
// session and cookie scope pointers plus the evaluated attributes (each
// attribute may be null when absent). Loads (or creates) the named application
// and, when sessionmanagement is enabled, the current CFID/CFTOKEN session,
// filling *application/*session so templates read/write the scopes directly.
cfvariant *cf_application_enable(cfvariant *application, cfvariant *session,
                                 const cfvariant *cookie,
                                 const cfvariant *name, const cfvariant *sessionManagement,
                                 const cfvariant *appTimeout, const cfvariant *sessionTimeout,
                                 const cfvariant *setClientCookies);

// ---- <cflogin> / <cfloginuser> / <cflogout> + auth functions runtime ----

// Per-request security state: the currently authenticated user of the request
// (CF's FusionContext secure table). The login tags fill it; GetAuthUser /
// IsUserLoggedIn / GetUserRoles / IsUserInRole / IsUserInAnyRole read it.
// Reset per request by security_begin (called from scope_begin).
struct SecurityContext {
    bool loggedIn = false;
    webstrada::string username;
    webstrada::string password;
    std::vector<webstrada::string> roles;   // in the order the login gave them
    webstrada::string appToken;             // application token this login belongs to
};

// Server-wide auth pool access (CF's SecurityScopeTracker + SecurityManager).
// Returns the auth token for the login, or an empty string when the pool store
// is unavailable. `appToken` is the application token the login is scoped to.
webstrada::string security_create_auth_token(const webstrada::string &username,
                                             const webstrada::string &appToken,
                                             const webstrada::string &password,
                                             int64_t maxInactiveMs);
// Resolve a token to a SecurityContext (validating the nonce and idle timeout).
// Returns true and fills *out when the token is live.
bool security_resolve_token(const webstrada::string &token, SecurityContext *out);
// Store the SecurityContext under `token` (idle timeout in ms; 0 = none).
void security_store_table(const webstrada::string &token, const SecurityContext &ctx,
                          int64_t maxInactiveMs);
// Remove the pool entry for `token` and any pool entries sharing appToken.
void security_remove_token(const webstrada::string &token);
void security_remove_by_app_token(const webstrada::string &appToken);

// <cflogin> start-tag runtime. Returns 1 to run the body (user not logged in
// or credentials submitted), 0 to skip it (already logged in). Pushes a login
// frame so <cfloginuser> inside the body binds to this tag. Resolves an
// existing login from the CFAUTHORIZATION cookie or (loginstorage="session")
// the session scope; the j_username/j_password fields populate the `cflogin`
// struct in the variables scope. Each absent attribute pointer may be null.
int cf_login_begin(string *out, void *cgi, void *server, void *cookie,
                   void *application, void *session, void *url, void *form,
                   void *variables,
                   const cfvariant *idletimeout, const cfvariant *usebasicauth,
                   const cfvariant *allowconcurrent, const cfvariant *applicationtoken,
                   const cfvariant *cookiedomain);

// <cflogin> end-tag runtime: commit a pending <cfloginuser> (creating the auth
// token + CFAUTHORIZATION cookie/session key) or log out when none ran, then
// remove the `cflogin` local. `runBody` is the value cf_login_begin returned.
void cf_login_end(string *out, void *cgi, void *server, void *cookie,
                  void *application, void *session, void *url, void *form,
                  void *variables,
                  const cfvariant *idletimeout, const cfvariant *usebasicauth,
                  const cfvariant *allowconcurrent, const cfvariant *applicationtoken,
                  const cfvariant *cookiedomain, int runBody);

// <cfloginuser> runtime: identifies the current user with the given roles.
// Inside a <cflogin> body the name/password/roles bind to that tag (committed
// at its end tag); outside one the request is logged in directly (no cookie).
void cf_loginuser(string *out, void *cgi, void *server, void *cookie,
                  void *application, void *session, void *url, void *form,
                  void *variables,
                  const cfvariant *name, const cfvariant *password,
                  const cfvariant *roles);

// <cflogout> runtime: log out the current user. `session` (current/all/others)
// and `applicationtoken` are optional; a null `session` means "current".
void cf_logout(string *out, void *cgi, void *server, void *cookie,
               void *application, void *session, void *url, void *form,
               void *variables,
               const cfvariant *sessionAttr, const cfvariant *applicationtoken);

// Auth functions (read the per-request SecurityContext installed by the login
// tags). GetAuthUser returns "" and IsUserLoggedIn NO when nobody is logged in.
cfvariant *cf_getauthuser();
cfvariant *cf_getuserroles();
cfvariant *cf_isuserloggedin();
cfvariant *cf_isuserinrole(const cfvariant *role);
cfvariant *cf_isuserinanyrole(const cfvariant *rolelist);

// Reset the per-request SecurityContext (called by scope_begin).
void security_reset();

void cfset(cfvariant *scope, const char *key, const char *value);

// Sets whether unqualified (unprefixed) name lookup also searches the implicit
// scopes (CGI, FILE, URL, FORM, COOKIE, CLIENT). Default false; ColdFusion's
// `<cfapplication searchimplicitscopes="yes">` enables it. SERVER / APPLICATION
// / SESSION are never searched for unqualified names regardless of this flag.
void cf_set_search_implicit_scopes(const cfvariant *value);

// <cfapplication loginstorage="session|cookie">: sets whether the CFAUTHORIZATION
// login key is stored in the session scope instead of a cookie. Default cookie.
void cf_set_login_storage(const cfvariant *value);

// <cfparam> runtime (ColdFusion's ParamTag). Finds the named parameter
// (variables scope + implicit CGI/URL/FORM/COOKIE scopes for unqualified
// names, like pageContext.findAttribute), assigns `defaultVal` when missing,
// then validates the value against `type` (any/array/boolean/date/.../regex/
// range), reproducing CF's errors byte-for-byte. Each absent attribute is
// null; `type` being null selects CF's checkSimpleParameter path (the missing
// parameter error reports the uppercased name). Returns the resulting value.
cfvariant *cf_param(const cfvariant *name, const cfvariant *type,
                    const cfvariant *defaultVal, const cfvariant *min,
                    const cfvariant *max, const cfvariant *maxlength,
                    const cfvariant *pattern,
                    void *cgi, void *server, void *cookie,
                    void *application, void *session, void *url,
                    void *form, void *variables);

// <cfobjectcache> runtime (ColdFusion's ObjectCacheTag): action "clear"
// (case-insensitive; the default when absent) flushes the query cache (the
// CacheStore QUERY region). Any other action throws CF's catchable Template
// "Attribute validation error for CFOBJECTCACHE." error.
void cf_objectcache(const cfvariant *action);

void cfoutputexpr(string &out, void *cgi, void *server, void *cookie,
                   void *application, void *session, void *url,
                   void *form, void *variables, const char *varName);

const cfvariant *cfgetvar(const cfvariant *scope, const char *key);

/**
 * @brief Evaluate a CFML boolean condition at runtime.
 *
 * Parses @p expr as a simple CFML condition (truthy check or binary
 * comparison with operators EQ/NEQ/GT/GTE/LT/LTE/IS/CONTAINS) and
 * returns the result as an integer (0 = false, 1 = true).
 */
int cfevalbool(const char *expr,
               const cfvariant *cgi, const cfvariant *server,
               const cfvariant *cookie, const cfvariant *application,
               const cfvariant *session, const cfvariant *url,
               const cfvariant *form, const cfvariant *variables);

/**
 * @brief Resolve a cfloop attribute (from/to/step) to an int at runtime.
 *
 * Handles plain numeric literals and #varname# variable references.
 * var name is looked up across all scopes and converted to int.
 */
int cfloop_resolve_int(const char *expr,
                       const cfvariant *cgi, const cfvariant *server,
                       const cfvariant *cookie, const cfvariant *application,
                       const cfvariant *session, const cfvariant *url,
                       const cfvariant *form, const cfvariant *variables);

/**
 * @brief Store an int as a Number variant in a scope (used by cfloop
 *        to update the index variable each iteration).
 */
void cfloop_set_int(cfvariant *scope, const char *key, int val);

/**
 * @brief Store a 64-bit loop index in a scope (used by cfloop to update
 *        the index variable each iteration).
 *
 * The target scope is resolved like an unqualified assignment (see
 * udfAssignScope): a `var`-declared loop index in a function is written to the
 * function's `local` scope, otherwise to the variables/parent scope. Values
 * that fit in a signed 32-bit range are stored as a Number variant; larger
 * ones are stored as a Float (double) so the index renders like a computed
 * double (CF renders cfloop bounds/indices beyond int32 as doubles).
 */
void cfloop_set_long(
    const cfvariant *cgi, const cfvariant *server,
    const cfvariant *cookie, const cfvariant *application,
    const cfvariant *session, const cfvariant *url,
    const cfvariant *form, cfvariant *variables,
    const char *key, long long val);

/**
 * @brief Assign a cfloop list/array/collection iteration value to the loop
 *        index variable, resolving the target scope like an unqualified
 *        assignment (udfAssignScope).
 */
void cfloop_assign_index(
    const cfvariant *cgi, const cfvariant *server,
    const cfvariant *cookie, const cfvariant *application,
    const cfvariant *session, const cfvariant *url,
    const cfvariant *form, cfvariant *variables,
    const char *name, const cfvariant *value);

/**
 * @brief Number of iterations a `for (x in coll)` loop will perform.
 *
 * Arrays iterate their elements, structs iterate their keys (uppercased as
 * stored, in insertion order), strings iterate as a list split on any
 * character in `delims` (CF's default list delimiter is ","). Queries/other
 * types return 0.
 */
long long cfforInLength(const cfvariant *coll, const cfvariant *delims);

/**
 * @brief The 1-based iteration item for a `for (x in coll)` loop.
 *
 * Arrays return a pointer to the element, structs return the key string,
 * strings return the list item at `index` split on any character in `delims`.
 * Throws when `index` is out of range or the collection type is not iterable.
 */
cfvariant *cfforInItem(const cfvariant *coll, long long index, const cfvariant *delims);


/**
 * @brief Abort request execution immediately.
 *
 * Throws an abort_exception to halt template processing.
 */
void cfabort(void);

/**
 * @brief <cfexit> / script `exit;` outside a function body.
 *
 * Throws an exit_exception to abort the currently executing template page
 * (uncatchable by CFML catch blocks; swallowed at include/construction/prelude
 * boundaries, halts the page at the top level).
 */
void cf_exit(const cfvariant *method);

/**
 * @brief <cfexit method="loop"> outside a custom tag.
 *
 * Throws CF's catchable InvalidExitLoopMethodException ("Invalid use of the
 * cfexit tag."). This engine has no custom tags, so it is always invalid.
 */
void cf_exit_loop(void);

/**
 * @brief Runtime method-attribute validation for a dynamically evaluated
 * method value: throws CF's catchable Template exception ("Attribute
 * validation error for CFEXIT.").
 */
void cf_exit_invalid(const cfvariant *method);

/**
 * @brief Classifies an evaluated `method` attribute value.
 *
 * @return 0 = exittag/exittemplate/empty (exit current page / return from
 * function), 1 = loop (invalid outside a custom tag), 2 = invalid method.
 */
int cf_exit_classify(const cfvariant *method);

/**
 * @brief Evaluate a CFML expression and return the result (test helper).
 *
 * Exposes the internal expression evaluator for unit testing.
 * Only function calls, not variable lookups or operators.
 *
 * @param[out] out        Output buffer (may be written to by functions like WriteOutput).
 * @param[in]  expr       The CFML expression to evaluate.
 * @param[in]  variables  The variables scope (may be nullptr).
 * @return The evaluated cfvariant value.
 */
cfvariant cfevaluate(string &out, const string &expr, cfvariant *variables);

// JIT expression resolver helper functions
cfvariant *cfvariant_create_null();
cfvariant *cfvariant_create_int(int val);
cfvariant *cfvariant_create_long(long long val);
cfvariant *cfvariant_create_float(double val);
cfvariant *cfvariant_create_float_literal(const char *text, double val);
cfvariant *cfvariant_create_bool(bool val);
// Creates a boolean that stringifies literally (true/false) — used for the
// source literals true/false/yes/no. Computed booleans (comparisons, not,
// boolean-returning functions) must use cfvariant_create_bool, whose result
// stringifies as YES/NO (see BUGS.md #7).
cfvariant *cfvariant_create_bool_literal(bool val);
cfvariant *cfvariant_create_string(const char *val);
cfvariant *cfvariant_create_array();
cfvariant *cfvariant_create_struct();
// Compiler-extension functions (the `__` prefix family, reserved like C's
// `__` identifiers). Uniform ABI: a cfvariant* argument array plus its count.
// Compiled as direct JIT calls to cf___<name>(args, argc) by the codegen.
cfvariant *cf___configget(const cfvariant **args, int argc);
cfvariant *cf___configset(const cfvariant **args, int argc);
cfvariant *cf___datasourcetest(const cfvariant **args, int argc);
cfvariant *cf___serverinfo(const cfvariant **args, int argc);
cfvariant *cf___configreset(const cfvariant **args, int argc);
cfvariant *cf___cacheinfo(const cfvariant **args, int argc);
cfvariant *cf___cacheevict(const cfvariant **args, int argc);
cfvariant *cf___cacheclear(const cfvariant **args, int argc);
// Registry for the `__` extension family. cf_is_extension_name validates a
// name (upper-cased) at compile time; cf_extension_call dispatches a call for
// the #...# interpreter. Both return false/nullptr for unregistered names.
bool cf_is_extension_name(const char *upperName);
cfvariant *cf_extension_call(const char *upperName, const cfvariant **args, int argc);
cfvariant *cf_abs(const cfvariant *arg);
cfvariant *cf_asc(const cfvariant *arg);
cfvariant *cf_chr(const cfvariant *arg);
cfvariant *cf_acos(const cfvariant *arg);
cfvariant *cf_asin(const cfvariant *arg);
cfvariant *cf_atan(const cfvariant *arg);
cfvariant *cf_atan2(const cfvariant *y, const cfvariant *x);
cfvariant *cf_ceiling(const cfvariant *arg);
cfvariant *cf_cos(const cfvariant *arg);
cfvariant *cf_exp(const cfvariant *arg);
cfvariant *cf_floor(const cfvariant *arg);
cfvariant *cf_incrementvalue(const cfvariant *arg);
cfvariant *cf_decrementvalue(const cfvariant *arg);
cfvariant *cf_int(const cfvariant *arg);
cfvariant *cf_log(const cfvariant *arg);
cfvariant *cf_log10(const cfvariant *arg);
cfvariant *cf_max(const cfvariant *arg1, const cfvariant *arg2);
cfvariant *cf_min(const cfvariant *arg1, const cfvariant *arg2);
cfvariant *cf_pi();
cfvariant *cf_rand(const cfvariant *algorithm);
cfvariant *cf_randomize(const cfvariant *arg, const cfvariant *algorithm);
cfvariant *cf_randrange(const cfvariant *number1, const cfvariant *number2, const cfvariant *algorithm);
cfvariant *cf_round(const cfvariant *arg);
cfvariant *cf_sgn(const cfvariant *arg);
cfvariant *cf_sin(const cfvariant *arg);
cfvariant *cf_sqr(const cfvariant *arg);
cfvariant *cf_tan(const cfvariant *arg);
cfvariant *cf_len(const cfvariant *arg);
cfvariant *cf_left(const cfvariant *str, const cfvariant *cnt);
cfvariant *cf_right(const cfvariant *str, const cfvariant *cnt);
cfvariant *cf_mid(const cfvariant *str, const cfvariant *startVal, const cfvariant *cnt);
cfvariant *cf_trim(const cfvariant *arg);
cfvariant *cf_ltrim(const cfvariant *arg);
cfvariant *cf_rtrim(const cfvariant *arg);
cfvariant *cf_lcase(const cfvariant *arg);
cfvariant *cf_ucase(const cfvariant *arg);
cfvariant *cf_reverse(const cfvariant *arg);
cfvariant *cf_repeatstring(const cfvariant *str, const cfvariant *cnt);
cfvariant *cf_replace(const cfvariant *str, const cfvariant *sub1Val, const cfvariant *sub2Val, const cfvariant *scopeVal);
cfvariant *cf_replacenocase(const cfvariant *str, const cfvariant *sub1Val, const cfvariant *sub2Val, const cfvariant *scopeVal);
cfvariant *cf_find(const cfvariant *subVal, const cfvariant *strVal, const cfvariant *startVal);
cfvariant *cf_findnocase(const cfvariant *subVal, const cfvariant *strVal, const cfvariant *startVal);
cfvariant *cf_compare(const cfvariant *s1Val, const cfvariant *s2Val);
cfvariant *cf_comparenocase(const cfvariant *s1Val, const cfvariant *s2Val);
cfvariant *cf_decimalformat(const cfvariant *arg);
cfvariant *cf_dollarformat(const cfvariant *arg);
cfvariant *cf_yesnoformat(const cfvariant *arg);
cfvariant *cf_now();
cfvariant *cf_createdatetime(const cfvariant *yrVal, const cfvariant *monVal, const cfvariant *dayVal, const cfvariant *hrVal, const cfvariant *minVal, const cfvariant *secVal);
cfvariant *cf_createdate(const cfvariant *yrVal, const cfvariant *monVal, const cfvariant *dayVal);
cfvariant *cf_createtime(const cfvariant *hrVal, const cfvariant *minVal, const cfvariant *secVal);
cfvariant *cf_isdate(const cfvariant *arg);
cfvariant *cf_year(const cfvariant *arg);
cfvariant *cf_month(const cfvariant *arg);
cfvariant *cf_day(const cfvariant *arg);
cfvariant *cf_hour(const cfvariant *arg);
cfvariant *cf_minute(const cfvariant *arg);
cfvariant *cf_second(const cfvariant *arg);

// Array JIT helper functions
cfvariant *cf_arraynew();
cfvariant *cf_arraylen(const cfvariant *arr);
cfvariant *cf_arrayfirst(const cfvariant *arr);
cfvariant *cf_arraylast(const cfvariant *arr);
cfvariant *cf_arraypop(cfvariant *arr);
cfvariant *cf_arrayshift(cfvariant *arr);
cfvariant *cf_arrayappend(cfvariant *arr, const cfvariant *val);
cfvariant *cf_arrayprepend(cfvariant *arr, const cfvariant *val);
cfvariant *cf_arrayisempty(const cfvariant *arr);
cfvariant *cf_arrayclear(cfvariant *arr);
cfvariant *cf_arraydeleteat(cfvariant *arr, const cfvariant *idx);
cfvariant *cf_arrayinsertat(cfvariant *arr, const cfvariant *idx, const cfvariant *val);
cfvariant *cf_arrayresize(cfvariant *arr, const cfvariant *sz);
cfvariant *cf_arrayset(cfvariant *arr, const cfvariant *start, const cfvariant *end, const cfvariant *val);
cfvariant *cf_arraytolist(const cfvariant *arr, const cfvariant *delim);
cfvariant *cf_arrayavg(const cfvariant *arr);
cfvariant *cf_arrayeach(const cfvariant *arr, const cfvariant *callback, const cfvariant *parallel, const cfvariant *maxThreads,
                        string &out, void *cgi, void *server, void *cookie, void *application, void *session, void *url, void *form, void *variables);
cfvariant *cf_arrayfilter(const cfvariant *arr, const cfvariant *callback, const cfvariant *parallel, const cfvariant *maxThreads,
                          string &out, void *cgi, void *server, void *cookie, void *application, void *session, void *url, void *form, void *variables);
cfvariant *cf_arrayreduce(const cfvariant *arr, const cfvariant *callback, const cfvariant *initialValue,
                          string &out, void *cgi, void *server, void *cookie, void *application, void *session, void *url, void *form, void *variables);
cfvariant *cf_arraysum(const cfvariant *arr);
cfvariant *cf_arraymin(const cfvariant *arr);
cfvariant *cf_arraymax(const cfvariant *arr);
cfvariant *cf_arraycontains(const cfvariant *arr, const cfvariant *val);
cfvariant *cf_arraycontainsnocase(const cfvariant *arr, const cfvariant *val);
cfvariant *cf_arrayfind(const cfvariant *arr, const cfvariant *val);
cfvariant *cf_arrayfindnocase(const cfvariant *arr, const cfvariant *val);
cfvariant *cf_arrayfindall(const cfvariant *arr, const cfvariant *val);
cfvariant *cf_arrayfindallnocase(const cfvariant *arr, const cfvariant *val);
cfvariant *cf_arraydelete(cfvariant *arr, const cfvariant *val);
cfvariant *cf_arraydeletenocase(cfvariant *arr, const cfvariant *val);
cfvariant *cf_arrayslice(const cfvariant *arr, const cfvariant *offset, const cfvariant *length);
cfvariant *cf_arrayswap(cfvariant *arr, const cfvariant *idx1, const cfvariant *idx2);
cfvariant *cf_isarray(const cfvariant *val);
cfvariant *cf_structvaluearray(const cfvariant *str);
cfvariant *cf_arrayisdefined(const cfvariant *arr, const cfvariant *idx);
cfvariant *cf_listtoarray(const cfvariant *list, const cfvariant *delim);

// File / Directory JIT helper functions
cfvariant *cf_fileexists(const cfvariant *path);
cfvariant *cf_directoryexists(const cfvariant *path);
cfvariant *cf_fileread(const cfvariant *path);
cfvariant *cf_filewrite(const cfvariant *path, const cfvariant *content);
cfvariant *cf_filedelete(const cfvariant *path);
cfvariant *cf_filecopy(const cfvariant *src, const cfvariant *dest);
cfvariant *cf_filemove(const cfvariant *src, const cfvariant *dest);
cfvariant *cf_directorycreate(const cfvariant *path);
cfvariant *cf_directorydelete(const cfvariant *path, const cfvariant *recurse);
cfvariant *cf_getfileinfo(const cfvariant *path);

// List JIT helper functions
cfvariant *cf_listlen(const cfvariant *list, const cfvariant *delim);
cfvariant *cf_listappend(const cfvariant *list, const cfvariant *val, const cfvariant *delim);
cfvariant *cf_listprepend(const cfvariant *list, const cfvariant *val, const cfvariant *delim);
cfvariant *cf_listinsertat(const cfvariant *list, const cfvariant *idx, const cfvariant *val, const cfvariant *delim);
cfvariant *cf_listdeleteat(const cfvariant *list, const cfvariant *idx, const cfvariant *delim);
cfvariant *cf_listgetat(const cfvariant *list, const cfvariant *idx, const cfvariant *delim);
cfvariant *cf_listsetat(const cfvariant *list, const cfvariant *idx, const cfvariant *val, const cfvariant *delim);
cfvariant *cf_listfirst(const cfvariant *list, const cfvariant *delim);
cfvariant *cf_listlast(const cfvariant *list, const cfvariant *delim);
cfvariant *cf_listrest(const cfvariant *list, const cfvariant *delim);
cfvariant *cf_listfind(const cfvariant *list, const cfvariant *val, const cfvariant *delim);
cfvariant *cf_listfindnocase(const cfvariant *list, const cfvariant *val, const cfvariant *delim);
cfvariant *cf_listcontains(const cfvariant *list, const cfvariant *val, const cfvariant *delim);
cfvariant *cf_listcontainsnocase(const cfvariant *list, const cfvariant *val, const cfvariant *delim);
cfvariant *cf_listchangedelims(const cfvariant *list, const cfvariant *newDelim, const cfvariant *delim);

// Struct JIT helper functions
cfvariant *cf_structnew();
cfvariant *cf_structinsert(cfvariant *str, const cfvariant *key, const cfvariant *val, const cfvariant *allowOverwrite);
cfvariant *cf_structupdate(cfvariant *str, const cfvariant *key, const cfvariant *val);
cfvariant *cf_structdelete(cfvariant *str, const cfvariant *key, const cfvariant *indicateExisting);
cfvariant *cf_structfind(const cfvariant *str, const cfvariant *key);
cfvariant *cf_structkeyexists(const cfvariant *str, const cfvariant *key);
cfvariant *cf_structisempty(const cfvariant *str);
cfvariant *cf_structcount(const cfvariant *str);
cfvariant *cf_structclear(cfvariant *str);
cfvariant *cf_structkeyarray(const cfvariant *str);
cfvariant *cf_structkeylist(const cfvariant *str, const cfvariant *delim);


const cfvariant *cfvariant_get_var(
    const cfvariant *cgi, const cfvariant *server,
    const cfvariant *cookie, const cfvariant *application,
    const cfvariant *session, const cfvariant *url,
    const cfvariant *form, const cfvariant *variables,
    const char *name);

// Compile-time-bound variable slot (ColdFusion's "fast path"). The JIT resolves
// each page-level variable name once at template entry to a slot; reads then
// short-circuit on the memoized pointer when the variables scope's generation
// still matches, falling back to a full scope search on a miss (only a miss
// walks the scope chain, like CF's bound Variable). The generation counter on
// StructData bumps on every erase/clear, so a cached pointer into the scope
// map is never read after the node is freed.
struct VarFastSlot {
    const cfvariant *ptr;   // resolved value in the variables scope, or null
    const StructData *sd;   // the variables scope's StructData the pointer lives in
    uint64_t gen;           // sd->generation captured when ptr was memoized
};

// Fast-path variants of cfvariant_get_var / cfvariant_bare_identifier. slot is
// the compile-time-bound slot for `name` (null to disable the fast path). Same
// error/fallback behavior as the slow versions on a cache miss.
const cfvariant *cfvariant_get_var_fast(
    VarFastSlot *slot,
    const cfvariant *cgi, const cfvariant *server,
    const cfvariant *cookie, const cfvariant *application,
    const cfvariant *session, const cfvariant *url,
    const cfvariant *form, const cfvariant *variables,
    const char *name);
cfvariant *cfvariant_bare_identifier_fast(
    VarFastSlot *slot,
    const cfvariant *cgi, const cfvariant *server,
    const cfvariant *cookie, const cfvariant *application,
    const cfvariant *session, const cfvariant *url,
    const cfvariant *form, const cfvariant *variables,
    const char *name);

// JIT helper for a DOT member access on a chain base (`s.key`). Resolves the
// base and reads the named member; on an undefined base it throws CF's ELEMENT
// message ("Element KEY is undefined in BASE.") rather than the variable
// message used by bracket access (was BUGS.md "chain-base lookups").
cfvariant *cfvariant_get_member(
    const cfvariant *cgi, const cfvariant *server,
    const cfvariant *cookie, const cfvariant *application,
    const cfvariant *session, const cfvariant *url,
    const cfvariant *form, const cfvariant *variables,
    const char *name, const char *key);

// CF reports a missing member of an enabled scope as "Element X is undefined in
// SESSION." (member path uppercased, scope uppercased). Throws CF's message and
// returns true when `name` is such a scope-qualified missing member; returns
// false otherwise so the caller falls back to its generic message.
bool cf_throw_scope_member_error(const char *name,
    const cfvariant *cgi, const cfvariant *server,
    const cfvariant *cookie, const cfvariant *application,
    const cfvariant *session, const cfvariant *url,
    const cfvariant *form, const cfvariant *variables);

// Throws CF's array out-of-bounds error. `varName` is the uppercased array
// variable name when the failing index is on a simple named variable (CF:
// "The element at position N of dimension D, of array variable &quot;NAME,&quot;
// cannot be found."); null for an array object used as part of an expression
// ("... of an array object used as part of an expression, cannot be found.").
[[noreturn]] void cf_throw_array_oob(int idx, int dimension, const char *varName);

// Like cfvariant_index but knows the base variable name and index dimension so
// an out-of-bounds read reports CF's named-variable message (was BUGS.md
// "Array index out-of-bounds error message"). varName is null to use the
// array-object form.
cfvariant *cfvariant_index_named(cfvariant *arr, const cfvariant *idx, const char *varName, int dimension);


// JIT helper for a bare identifier in a cfscript expression. Mirrors the
// interpreter's evaluateExpr step 9 (previously BUGS.md #2): a variable
// shadows a built-in function of the same name; otherwise a known CFML
// built-in function name resolves to a method handle
// (coldfusion.runtime.CFPageMethod@<hash>); otherwise it throws
// "Variable X is undefined.".
cfvariant *cfvariant_bare_identifier(
    const cfvariant *cgi, const cfvariant *server,
    const cfvariant *cookie, const cfvariant *application,
    const cfvariant *session, const cfvariant *url,
    const cfvariant *form, const cfvariant *variables,
    const char *name);

cfvariant *cfvariant_assign(
    const cfvariant *cgi, const cfvariant *server,
    const cfvariant *cookie, const cfvariant *application,
    const cfvariant *session, const cfvariant *url,
    const cfvariant *form, cfvariant *variables,
    const char *name, const cfvariant *value);

cfvariant *cfvariant_add(const cfvariant *a, const cfvariant *b);
cfvariant *cfvariant_sub(const cfvariant *a, const cfvariant *b);
cfvariant *cfvariant_mul(const cfvariant *a, const cfvariant *b);
cfvariant *cfvariant_div(const cfvariant *a, const cfvariant *b);
cfvariant *cfvariant_mod(const cfvariant *a, const cfvariant *b);
cfvariant *cfvariant_idiv(const cfvariant *a, const cfvariant *b);
cfvariant *cfvariant_pow(const cfvariant *a, const cfvariant *b);
cfvariant *cfvariant_neg(const cfvariant *a);
cfvariant *cfvariant_concat(const cfvariant *a, const cfvariant *b);

cfvariant *cfvariant_and(const cfvariant *a, const cfvariant *b);
cfvariant *cfvariant_or(const cfvariant *a, const cfvariant *b);
cfvariant *cfvariant_xor(const cfvariant *a, const cfvariant *b);
cfvariant *cfvariant_not(const cfvariant *a);

cfvariant *cfvariant_compare(const cfvariant *a, const cfvariant *b, const char *op);

// CF's script `?:` ternary truthiness (CfJspPage._isTruthyValue), a different
// rule than Cast._boolean: null->false, Boolean->value, Integer->!=0, String
// false only for "false"/"no"/"0" (empty string is true), else true.
int cf_is_truthy_value(const cfvariant *v);
// Selects the taken ternary branch (evaluates both args already).
cfvariant *cf_ternary_select(const cfvariant *cond, const cfvariant *thenV, const cfvariant *elseV);

cfvariant *cfvariant_index(cfvariant *arr, const cfvariant *idx);
cfvariant *cfvariant_index_assign(cfvariant *arr, const cfvariant *idx, const cfvariant *val);
// Nested index assignment a[i1][i2]...[in] = v, auto-creating missing
// intermediate array rows like ColdFusion (a = ArrayNew(2); a[1][5] = 1).
cfvariant *cfvariant_index_assign_deep(cfvariant *root, const cfvariant **idxChain, int n, const cfvariant *val);

/**
 * @brief Deep-copy a cfvariant value into a fresh temporary variant.
 * Used so a post-increment (x++) can evaluate to the old value even though
 * cfvariant_assign writes the new value into the same slot.
 */
cfvariant *cfvariant_copy_value(const cfvariant *a);
cfvariant *cfvariant_call_function(
    string &out,
    void *cgi, void *server, void *cookie, void *application, void *session, void *url, void *form, void *variables,
    const char *name, const cfvariant **args, int arg_count);

// Dispatches a member-method call `base.method(args)`. Resolves a stored
// callable (a UDF/closure held in a struct/xml member) first, then falls back
// to the built-in member-method table (which maps the method name to the
// standalone function with the receiver prepended as the first argument).
// Throws when the base has no such member/method. Returns a temp value.
cfvariant *cfvariant_member_method(
    cfvariant *base, const char *name,
    const cfvariant **args, int arg_count,
    string &out, void *cgi, void *server, void *cookie, void *application, void *session, void *url, void *form, void *variables);

// ---- User-defined functions and closures ----

// JIT entry-point signature of a compiled UDF/closure body. The body creates
// its own local scope; `parentScope` is the captured enclosing scope,
// `args`/`argc` the passed arguments. Returns the (heap) return value.
typedef cfvariant *(*udf_entry_fn)(
    string *out, void *cgi, void *server, void *cookie, void *application,
    void *session, void *url, void *form,
    cfvariant *parentScope,
    const cfvariant **args, int argc);

// Creates a callable Function value carrying a JIT UDF/closure entry point.
// `capturedScope` is borrowed (owned by the request cleanup). `metaBlob` is the
// serialized UdfMetaInfo (see cfvariant.h; nullable for built-in method
// handles). Returns a temp.
cfvariant *cfvariant_create_udf(const char *name, void *fn, bool isClosure, cfvariant *capturedScope, const void *metaBlob);

// UDF invocation scope-context management (thread-local stack).
void cf_udf_begin(cfvariant *localScope, cfvariant *parentScope);
void cf_udf_mark_local(const char *name);
void cf_udf_end();
// Clear partially-unwound UDF contexts at a request boundary.
void cf_udf_context_clear();

// Invokes a Function value with the given args (temp-returned). Throws when the
// value is not a callable function.
cfvariant *cf_udf_invoke(cfvariant *udfVal, const cfvariant **args, int argc,
                         string &out, void *cgi, void *server, void *cookie, void *application,
                         void *session, void *url, void *form, void *variables);

// The reserved key (a NUL-prefixed name that cannot collide with a CFML
// identifier) marking a call's leading "named arguments" struct. The call site
// passes such a struct as args[0] — holding {NAME: value, ...} under this key —
// followed by the positional arguments, so the runtime can bind by name.
#define CFML_NAMED_ARGS_KEY "\x01" "CFML_NAMED_ARGS"

// The reserved key marking named arguments that do not match any declared
// parameter. ColdFusion silently accepts extra named arguments and exposes
// them in the function's `arguments` scope (verified on CF 2025:
// `g(x=1, y=2)` where g declares x returns arguments keys `X,y`, and
// `f(a=1, b=2)` where f declares nothing returns `a,b`). The reorder pass
// collects them under this key so cf_udf_build_arguments can merge them into
// the arguments struct under their original casing.
#define CFML_EXTRA_NAMED_ARGS_KEY "\x01" "CFML_EXTRA_NAMED_ARGS"

// Reorders a call's arguments against a function's declared parameter names.// When args[0] is the named-arguments marker (see CFML_NAMED_ARGS_KEY), the
// named pairs are placed into their matching parameter slots and the remaining
// positional args fill the unfilled slots in order; otherwise the array is
// returned unchanged. `out` receives the (possibly reordered) argument vector
// and `outArgc` its length. Returns true when named arguments were present.
bool cf_named_args_reorder(const cfvariant **args, int argc,
                           const char **paramNames, int paramCount,
                           std::vector<const cfvariant*> &out, int &outArgc);

// Builds a named-arguments marker struct (see CFML_NAMED_ARGS_KEY) wrapping the
// given {NAME: value} struct (temp-returned).
cfvariant *cf_named_args_marker(cfvariant *namedStruct);

// Type coercion for declared parameter/return types (throws the CF-matching
// InvalidArgumentTypeException / InvalidReturnTypeException / IllegalReturnException
// messages on non-convertible values). Returns a new temp value.
cfvariant *cf_udf_coerce_arg(const cfvariant *val, const char *typeName, const char *argName, const char *funcName);
cfvariant *cf_udf_coerce_return(cfvariant *val, const char *returnType, const char *funcName);

// Returns the cfvariant::cfvariantType enum value of a variant.
int cfvariant_type(const cfvariant *v);

// Throws CF's MissingArgumentException for a required <cfargument> not passed in.
void cf_throw_missing_argument(const char *paramName, const char *funcName);

// Builds the `arguments` struct (named param keys + positional keys + metadata)
// and stores it into the local scope under "ARGUMENTS".
void cf_udf_build_arguments(cfvariant *localScope, const char **paramNames, int paramCount,
                            const cfvariant **args, int argc);
// Removes the parameter keys from the local scope. CF's `local` scope holds
// only var-declared names + ARGUMENTS — function arguments live in the
// `arguments` scope (see PROGRESS.md cffunction row); the JIT prologue binds
// parameters into the local scope temporarily so cf_udf_build_arguments can
// copy them, then calls this before the body runs.
void cf_udf_remove_params(cfvariant *localScope, const char **paramNames, int paramCount);
// Stores a key/value pair into an arguments struct, writing a Null value when
// the source pointer is null (an unbound missing parameter).
void cf_udf_args_set_or_null(cfvariant *arguments, const char *key, cfvariant *val);
// Marks a scope struct as the `arguments` scope with the given param count.
void cf_udf_args_metadata(cfvariant *arguments, int paramCount);

// Registers a heap scope/value with the request's temporary-variant cleanup.
void cf_udf_register_temp(cfvariant *val);

// True when `name` (case-insensitive) is a CFML built-in function name.
bool cf_is_known_function_name(const char *name);

// ---- ColdFusion Components (CFCs) ----

// JIT entry signature of a compiled component method body. The method is
// compiled directly (no dynamic lookup): `variablesScope` is the component
// instance's `variables` scope, `thisScope` its `this` scope, `component` the
// ComponentInstance itself. The body creates its own function-local scope and
// pushes a component call context (cf_component_udf_begin) so unqualified
// names resolve local -> variables -> this and `this`/`variables` work.
typedef cfvariant *(*component_method_entry_fn)(
    string *out, void *cgi, void *server, void *cookie, void *application,
    void *session, void *url, void *form,
    cfvariant *variablesScope, cfvariant *thisScope, void *component,
    const cfvariant **args, int argc);

// JIT entry signature of a compiled component construction body (the top-level
// statements of a .cfc: this.x = ..., variables.y = ..., <cfset>...). Runs with
// the instance's scopes during instantiation.
typedef void (*component_body_fn)(
    string *out, void *cgi, void *server, void *cookie, void *application,
    void *session, void *url, void *form,
    cfvariant *variablesScope, cfvariant *thisScope);

// Pushes a component-method/construction call context (compiled component
// method prologue / instantiation helper): unqualified names then resolve
// local -> variables (the component's) -> this, and `this` works.
void cf_component_udf_begin(cfvariant *localScope, cfvariant *variablesScope,
                            cfvariant *thisScope, void *component);
void cf_udf_end();

// Creates a live component instance from a compiled definition and returns it
// as a Component cfvariant (temp). Runs the parent (extends) construction
// bodies then this component's, in that order, and applies <cfproperty>
// defaults. `variables` is the instantiating template's page variables scope.
cfvariant *cf_component_instantiate(webstrada::ComponentInfo *info,
                                    cfvariant *variables,
                                    string *out, void *cgi, void *server, void *cookie,
                                    void *application, void *session, void *url, void *form);

// Releases a ComponentInfo acquired via cf_component_load().
void cf_component_info_release(webstrada::ComponentInfo *info);

// Loads the component definition for a dot/relative path (CreateObject /
// <cfobject> / `new`): resolves `path` against the current template directory
// (absolute against the web root), compiles the .cfc via the installed
// component loader and resolves the `extends` chain. Returns a retained
// ComponentInfo (caller releases) or null when the file does not exist.
webstrada::ComponentInfo *cf_component_load(const char *path);

// Invokes a component method by name on a component value (used by
// `obj.method()` member dispatch and <cfinvoke>). Enforces CF access control
// (private methods are not callable from outside). Returns a temp.
cfvariant *cf_component_invoke(cfvariant *compVal, const char *methodName,
                               const cfvariant **args, int argc,
                               string &out, void *cgi, void *server, void *cookie,
                               void *application, void *session, void *url, void *form);

// Invokes a component method on a method-handle Function value (component
// methods referenced as values, e.g. `f = obj.method; f(...)`). Internal
// (already access-checked) invocation; used by cf_udf_invoke.
cfvariant *cf_component_method_handle_invoke(cfvariant *handleVal,
                                             const cfvariant **args, int argc,
                                             string &out, void *cgi, void *server, void *cookie,
                                             void *application, void *session, void *url, void *form);

// Invokes a method on a ComponentInstance by name WITHOUT access enforcement
// (internal component calls: a method calling another method, private or not).
cfvariant *cf_component_invoke_instance(webstrada::ComponentInstance *inst, const char *methodName,
                                        const cfvariant **args, int argc,
                                        string &out, void *cgi, void *server, void *cookie,
                                        void *application, void *session, void *url, void *form);

// Creates a callable Function value for a component method (method reference).
cfvariant *cf_component_method_handle(cfvariant *compVal, int methodIndex);

// True when a component value has a (public) method with this name.
int cf_component_has_method(const cfvariant *compVal, const char *methodName);

// True when a component instance has a method with this name (any access).
int cf_component_has_method_on(webstrada::ComponentInstance *inst, const char *methodName);

// Adds the component's public method names (uppercased, in declaration order)
// to the given key list (StructKeyList / StructKeyArray virtual view).
void cf_component_append_method_keys(const cfvariant *compVal, std::vector<webstrada::string> &keys);

// The CF "method not found in component" error for an inaccessible/unknown method.
[[noreturn]] void cf_component_throw_method_not_found(const cfvariant *compVal, const char *methodName);

// cfscript `new path.Component(args)`: loads the component, instantiates it and
// (when the component defines `init`) calls init with the given args, exactly
// like ColdFusion. Returns the component (temp).
cfvariant *cf_component_new(const char *path, const cfvariant **args, int argc,
                            string &out, void *cgi, void *server, void *cookie,
                            void *application, void *session, void *url, void *form,
                            void *variables);

// Returns the super scope of the current component method call context.
cfvariant *cf_component_get_super_scope();

// <cfobject type="component">: instantiate the component and store it in the
// `name` variable (cfvariant_assign). Throws for unsupported object types.
void cf_cfobject(string *out, void *cgi, void *server, void *cookie, void *application,
                 void *session, void *url, void *form, void *variables,
                 const cfvariant *type, const cfvariant *name, const cfvariant *component);

// <cfinvoke>: invoke a component method. `component` is a component value or a
// component path string; `method` the method name; `returnvariable` (optional)
// where the result is stored; `argumentcollection` (optional) a struct of named
// arguments. Returns the result (temp).
cfvariant *cf_cfinvoke(string *out, void *cgi, void *server, void *cookie, void *application,
                       void *session, void *url, void *form, void *variables,
                       const cfvariant *component, const cfvariant *method,
                       const cfvariant *returnvariable, const cfvariant *argumentcollection);

// <cfinvoke> ... </cfinvoke> with <cfinvokeargument> children: the start tag
// pushes a call context (cf_cfinvoke_begin), each <cfinvokeargument> appends a
// named argument (cf_cfinvoke_argument) and the end tag performs the invoke
// (cf_cfinvoke_end, which also assigns `returnvariable`). With no `component`
// attribute the method is resolved as a UDF in the page variables scope, like
// CF's InvokeTag. Returns the result (temp).
void cf_cfinvoke_begin(const cfvariant *component, const cfvariant *method,
                       const cfvariant *returnvariable, const cfvariant *argumentcollection);
void cf_cfinvoke_argument(const cfvariant *name, const cfvariant *value);
cfvariant *cf_cfinvoke_end(string *out, void *cgi, void *server, void *cookie, void *application,
                           void *session, void *url, void *form, void *variables);

// <cfimport path="...">: register a component import path (a dotted path or a
// "prefix.*" wildcard) that CreateObject/`new` fall back to. The taglib/prefix
// form is handled at compile time (see the custom-tag runtime in tag_custom.cpp
// and codegen_tags.cpp); this runtime stub is a no-op.
void cf_import_path(const cfvariant *path);

// <cfimport taglib/prefix>: processed statically by the compiler (registers the
// prefix->taglib mapping); the runtime stub is a no-op.
void cf_import_taglib(const cfvariant *taglib, const cfvariant *prefix);

// Legacy <cfmodule> runtime stub (unused — the compiler emits
// cf_custom_tag_invoke via compile_cfmodule_statement).
void cf_cfmodule(const cfvariant *templateAttr, const cfvariant *nameAttr,
                 const cfvariant *attributecollection);
void cf_cfassociate(const cfvariant *basetag, const cfvariant *datacollection);

size_t cfvariant_cleanup_save();
void cfvariant_cleanup_restore(size_t savepoint);
void cfvariant_cleanup_restore_except(size_t savepoint, cfvariant *retVal);

int cfvariant_is_truthy(const cfvariant *v);
int cfvariant_to_int(const cfvariant *v);
long long cfvariant_to_long(const cfvariant *v);

// Reset per-request cfdump page state (style block emission, pending abort).
void cfdump_reset_page();

struct VariantCleanupGuard {
    size_t savepoint;
    VariantCleanupGuard() {
        savepoint = cfvariant_cleanup_save();
        cfdump_reset_page();
    }
    ~VariantCleanupGuard() {
        cfvariant_cleanup_restore(savepoint);
    }
};


// Unimplemented CFML Function stubs throwing Not Implemented exceptions
cfvariant *cf_addsoaprequestheader();
cfvariant *cf_addsoapresponseheader();
cfvariant *cf_ajaxlink(const cfvariant *url);
cfvariant *cf_ajaxonload(const cfvariant *functionName, string &out);
cfvariant *cf_applicationstop();
cfvariant *cf_authenticatedcontext();
cfvariant *cf_authenticateduser();
cfvariant *cf_binarydecode(const cfvariant *str, const cfvariant *encoding);
cfvariant *cf_binaryencode(const cfvariant *binaryData, const cfvariant *encoding);
cfvariant *cf_bitand(const cfvariant *n1, const cfvariant *n2);
cfvariant *cf_bitmaskclear(const cfvariant *num, const cfvariant *start, const cfvariant *len);
cfvariant *cf_bitmaskread(const cfvariant *num, const cfvariant *start, const cfvariant *len);
cfvariant *cf_bitmaskset(const cfvariant *num, const cfvariant *mask, const cfvariant *start, const cfvariant *len);
cfvariant *cf_bitnot(const cfvariant *num);
cfvariant *cf_bitor(const cfvariant *n1, const cfvariant *n2);
cfvariant *cf_bitshln(const cfvariant *num, const cfvariant *count);
cfvariant *cf_bitshrn(const cfvariant *num, const cfvariant *count);
cfvariant *cf_bitxor(const cfvariant *n1, const cfvariant *n2);
cfvariant *cf_booleanformat(const cfvariant *val);
cfvariant *cf_formatbasen(const cfvariant *num, const cfvariant *radix);
cfvariant *cf_inputbasen(const cfvariant *str, const cfvariant *radix);
cfvariant *cf_jsstringformat(const cfvariant *str);
cfvariant *cf_removechars(const cfvariant *str, const cfvariant *start, const cfvariant *count);
cfvariant *cf_spanexcluding(const cfvariant *str, const cfvariant *set);
cfvariant *cf_spanincluding(const cfvariant *str, const cfvariant *set);
cfvariant *cf_stripcr(const cfvariant *str);
cfvariant *cf_cacheget(const cfvariant *id, const cfvariant *region = nullptr);
cfvariant *cf_cachegetallids(const cfvariant *region = nullptr, const cfvariant *includeExpired = nullptr);
cfvariant *cf_cachegetmetadata(const cfvariant *id, const cfvariant *objectType = nullptr, const cfvariant *region = nullptr);
cfvariant *cf_cachegetproperties(const cfvariant *region = nullptr);
cfvariant *cf_cachegetsession(const cfvariant *objectType, const cfvariant *isKey = nullptr);
cfvariant *cf_cacheidexists(const cfvariant *id, const cfvariant *region = nullptr);
cfvariant *cf_cacheput(const cfvariant *id, const cfvariant *value, const cfvariant *timespan = nullptr,
                       const cfvariant *idleTime = nullptr, const cfvariant *region = nullptr,
                       const cfvariant *throwOnError = nullptr);
cfvariant *cf_cacheregionexists(const cfvariant *region);
cfvariant *cf_cacheregionnew(const cfvariant *region, const cfvariant *properties = nullptr,
                              const cfvariant *throwOnError = nullptr);
cfvariant *cf_cacheregionremove(const cfvariant *region);
cfvariant *cf_cacheremove(const cfvariant *id, const cfvariant *throwOnError = nullptr,
                          const cfvariant *region = nullptr, const cfvariant *exact = nullptr);
cfvariant *cf_cacheremoveall(const cfvariant *region = nullptr);
cfvariant *cf_cachesetproperties(const cfvariant *properties, const cfvariant *region = nullptr);
cfvariant *cf_removecachedquery(const cfvariant *sql, const cfvariant *datasource = nullptr,
                                const cfvariant *params = nullptr, const cfvariant *region = nullptr);
cfvariant *cf_callstackget();
cfvariant *cf_callstackdump(string *out, const cfvariant *output = nullptr);
cfvariant *cf_candeserialize();

cfvariant *cf_canonicalize(const cfvariant *input, const cfvariant *restrictMultiple, const cfvariant *restrictMixed, const cfvariant *throwOnError = nullptr);
cfvariant *cf_canserialize();
cfvariant *cf_charsetdecode(const cfvariant *str, const cfvariant *encoding);
cfvariant *cf_charsetencode(const cfvariant *binaryData, const cfvariant *encoding);
cfvariant *cf_cjustify(const cfvariant *str, const cfvariant *length);
cfvariant *cf_createencryptedjwt();
cfvariant *cf_createobject(const cfvariant **args, int argc,
                           string &out, void *cgi, void *server, void *cookie,
                           void *application, void *session, void *url, void *form,
                           void *variables);
cfvariant *cf_createodbcdate(const cfvariant *date);
cfvariant *cf_createodbcdatetime(const cfvariant *date);
cfvariant *cf_createodbctime(const cfvariant *date);
cfvariant *cf_createsignedjwt();
cfvariant *cf_createtimespan(const cfvariant *days, const cfvariant *hours, const cfvariant *minutes, const cfvariant *seconds);
cfvariant *cf_csrfgeneratetoken(const cfvariant *key = nullptr, const cfvariant *forceNew = nullptr);
cfvariant *cf_csrfverifytoken(const cfvariant *token, const cfvariant *key = nullptr);
cfvariant *cf_csvprocess(const cfvariant *data, const cfvariant *callback, const cfvariant *delimiter,
                         string &out, void *cgi, void *server, void *cookie,
                         void *application, void *session, void *url, void *form,
                         void *variables);
cfvariant *cf_csvread(const cfvariant *data, const cfvariant *columns = nullptr, const cfvariant *delimiter = nullptr, const cfvariant *charset = nullptr);
cfvariant *cf_csvwrite(const cfvariant *data, const cfvariant *delimiter = nullptr);
cfvariant *cf_dateadd(const cfvariant *datepart, const cfvariant *number, const cfvariant *date);
cfvariant *cf_datecompare(const cfvariant *date1, const cfvariant *date2, const cfvariant *datePart = nullptr);
cfvariant *cf_dateconvert(const cfvariant *type, const cfvariant *date);
cfvariant *cf_datediff(const cfvariant *datepart, const cfvariant *date1, const cfvariant *date2);
cfvariant *cf_datepart(const cfvariant *datepart, const cfvariant *date);
cfvariant *cf_dayofweek(const cfvariant *date);
cfvariant *cf_dayofweekasstring(const cfvariant *dayOfWeek, const cfvariant *locale = nullptr);
cfvariant *cf_dayofyear(const cfvariant *date);
cfvariant *cf_daysinmonth(const cfvariant *date);
cfvariant *cf_daysinyear(const cfvariant *date);
cfvariant *cf_de(const cfvariant *str);
cfvariant *cf_decodeforhtml(const cfvariant *str);
cfvariant *cf_decodefromurl(const cfvariant *str);
cfvariant *cf_decrypt(const cfvariant *str, const cfvariant *key, const cfvariant *algorithm, const cfvariant *encoding, const cfvariant *IVorSalt, const cfvariant *iterations);
cfvariant *cf_decryptbinary(const cfvariant *binaryData, const cfvariant *key, const cfvariant *algorithm, const cfvariant *encoding, const cfvariant *IVorSalt, const cfvariant *iterations);
cfvariant *cf_deleteclientvariable(const cfvariant *name);
cfvariant *cf_deserialize(const cfvariant *data, const cfvariant *type);
cfvariant *cf_deserializejson(const cfvariant *jsonArg, const cfvariant *strictMappingArg = nullptr, bool literalBooleans = false);
cfvariant *cf_deserializexml(const cfvariant *arg0, const cfvariant *arg1 = nullptr, const cfvariant *arg2 = nullptr);
cfvariant *cf_directorycopy(const cfvariant *source, const cfvariant *destination);
cfvariant *cf_directorylist(const cfvariant *path, const cfvariant *recurse = nullptr, const cfvariant *filter = nullptr, const cfvariant *sort = nullptr, const cfvariant *type = nullptr);
cfvariant *cf_directoryrename(const cfvariant *source, const cfvariant *destination);
cfvariant *cf_dotnettocftype();
cfvariant *cf_duplicate(const cfvariant *obj);
cfvariant *cf_encodeforcss(const cfvariant *str, const cfvariant *canonicalize = nullptr);
cfvariant *cf_encodefordn(const cfvariant *str, const cfvariant *canonicalize = nullptr);
cfvariant *cf_encodeforhtml(const cfvariant *str, const cfvariant *canonicalize = nullptr);
cfvariant *cf_encodeforhtmlattribute(const cfvariant *str, const cfvariant *canonicalize = nullptr);
cfvariant *cf_encodeforjavascript(const cfvariant *str, const cfvariant *canonicalize = nullptr);
cfvariant *cf_encodeforldap(const cfvariant *str, const cfvariant *canonicalize = nullptr);
cfvariant *cf_encodeforurl(const cfvariant *str, const cfvariant *canonicalize);
cfvariant *cf_encodeforxml(const cfvariant *str, const cfvariant *canonicalize = nullptr);
cfvariant *cf_encodeforxmlattribute(const cfvariant *str, const cfvariant *canonicalize = nullptr);
cfvariant *cf_encodeforxpath(const cfvariant *str, const cfvariant *canonicalize = nullptr);
cfvariant *cf_encrypt(const cfvariant *str, const cfvariant *key, const cfvariant *algorithm, const cfvariant *encoding, const cfvariant *IVorSalt, const cfvariant *iterations);
cfvariant *cf_encryptbinary(const cfvariant *binaryData, const cfvariant *key, const cfvariant *algorithm, const cfvariant *encoding, const cfvariant *IVorSalt, const cfvariant *iterations);
cfvariant *cf_entitydelete();
cfvariant *cf_entityload();
cfvariant *cf_entityloadbyexample();
cfvariant *cf_entityloadbypk();
cfvariant *cf_entitymerge();
cfvariant *cf_entitynew();
cfvariant *cf_entityreload();
cfvariant *cf_entitysave();
cfvariant *cf_entitytoquery();
cfvariant *cf_evaluate(string &out, const cfvariant **args, int arg_count,
                       void *cgi, void *server, void *cookie,
                       void *application, void *session, void *url,
                       void *form, void *variables);
cfvariant *cf_expandpath(const cfvariant *path);
cfvariant *cf_fileclose(const cfvariant *fileObj);
cfvariant *cf_filegetmimetype(const cfvariant *path);
cfvariant *cf_fileiseof(const cfvariant *fileObj);
cfvariant *cf_fileopen(const cfvariant *path, const cfvariant *mode, const cfvariant *charset = nullptr);
cfvariant *cf_filereadbinary(const cfvariant *path);
cfvariant *cf_filereadline(const cfvariant *fileObj);
cfvariant *cf_fileseek(const cfvariant *fileObj, const cfvariant *position);
cfvariant *cf_filesetaccessmode(const cfvariant *path, const cfvariant *mode);
cfvariant *cf_filesetattribute(const cfvariant *path, const cfvariant *attr);
cfvariant *cf_filesetlastmodified(const cfvariant *path, const cfvariant *date);
cfvariant *cf_fileskipbytes(const cfvariant *fileObj, const cfvariant *count);
cfvariant *cf_fileupload(const cfvariant *dest, const cfvariant *fileField, const cfvariant *mimeType = nullptr, const cfvariant *onConflict = nullptr, const cfvariant *strict = nullptr);
cfvariant *cf_fileuploadall(const cfvariant *variables, const cfvariant *dest, const cfvariant *mimeType = nullptr, const cfvariant *onConflict = nullptr, const cfvariant *strict = nullptr, const cfvariant *continueOnError = nullptr, const cfvariant *errorVariable = nullptr, const cfvariant *allowedExtensions = nullptr);
cfvariant *cf_filewriteline(const cfvariant *fileObj, const cfvariant *content);
cfvariant *cf_getprofilesections(const cfvariant *iniPath);
cfvariant *cf_getprofilestring(const cfvariant *iniPath, const cfvariant *section, const cfvariant *key);
cfvariant *cf_findoneof(const cfvariant *set, const cfvariant *str, const cfvariant *start = nullptr);
cfvariant *cf_firstdayofmonth(const cfvariant *date);
cfvariant *cf_fix(const cfvariant *num);
cfvariant *cf_formatbasen();
cfvariant *cf_generate3deskey(const cfvariant *seed);
cfvariant *cf_generatepbkdfkey(const cfvariant *algorithm, const cfvariant *passphrase, const cfvariant *salt, const cfvariant *iterations, const cfvariant *keySize);
cfvariant *cf_generatesamlspmetadata();
cfvariant *cf_generatesecretkey(const cfvariant *algorithm, const cfvariant *keySize);
cfvariant *cf_getapplicationmetadata();
cfvariant *cf_getbasetagdata(const cfvariant *tagname = nullptr, const cfvariant *instancenumber = nullptr);
cfvariant *cf_getbasetaglist();
cfvariant *cf_getclientvariableslist();
cfvariant *cf_getcomponentmetadata(const cfvariant *obj);
cfvariant *cf_getcontextroot();
cfvariant *cf_getcpuusage(const cfvariant *interval = nullptr);
cfvariant *cf_getcspnonce();
cfvariant *cf_getencoding(const cfvariant *scope);
cfvariant *cf_getexception(const cfvariant *object);
cfvariant *cf_getfreespace(const cfvariant *path);
cfvariant *cf_getfunctioncalledname();
cfvariant *cf_getfunctionlist();
// Names of the built-in CFML functions backed by GetFunctionList() (enumerates
// the engine's registry, kBuiltinFunctionNames in core_udf.cpp).
std::vector<std::string> builtinFunctionNames();
cfvariant *cf_getgatewayhelper();
cfvariant *cf_gethttprequestdata(void *cgi, const cfvariant *includeBody);

// Per-request raw HTTP request body captured by the daemon worker (the CLI has
// none). cf_gethttprequestdata reads it for the `content` key. Reset per
// request (worker calls request_set_body with null at request start).
void request_set_body(const char *data, size_t len);
cfvariant *cf_gethttptimestring(const cfvariant *date = nullptr);
cfvariant *cf_getk2serverdoccount();
cfvariant *cf_getk2serverdoccountlimit();
cfvariant *cf_getlocaledisplayname(const cfvariant *locale = nullptr, const cfvariant *inLocale = nullptr);
cfvariant *cf_getlocalhostip();
cfvariant *cf_getmetadata(const cfvariant *obj);
cfvariant *cf_getmetricdata(const cfvariant *mode);
cfvariant *cf_getpagecontext();
cfvariant *cf_getprinterinfo();
cfvariant *cf_getprinterlist();
cfvariant *cf_getpropertyfile(const cfvariant *filePath, const cfvariant *encoding = nullptr);
cfvariant *cf_getpropertystring(const cfvariant *filePath, const cfvariant *key, const cfvariant *encoding = nullptr);
cfvariant *cf_getreadableimageformats();
cfvariant *cf_getsafehtml();cfvariant *cf_getsamlauthrequest();
cfvariant *cf_getsamllogoutrequest();
cfvariant *cf_getsoaprequest();
cfvariant *cf_getsoaprequestheader();
cfvariant *cf_getsoapresponse();
cfvariant *cf_getsoapresponseheader();
cfvariant *cf_getsystemfreememory();
cfvariant *cf_getsystemtotalmemory();
cfvariant *cf_gettimezoneinfo();
cfvariant *cf_gettoken(const cfvariant *str, const cfvariant *index, const cfvariant *delimiters);
cfvariant *cf_gettotalspace(const cfvariant *path);
cfvariant *cf_getvfsmetadata();
cfvariant *cf_getwriteableimageformats();
cfvariant *cf_hash(const cfvariant *str, const cfvariant *algorithm, const cfvariant *encoding, const cfvariant *additionalIterations);
cfvariant *cf_hmac(const cfvariant *message, const cfvariant *key, const cfvariant *algorithm, const cfvariant *encoding);
cfvariant *cf_hqlmethods();
cfvariant *cf_htmlcodeformat(const cfvariant *str, const cfvariant *version = nullptr);
cfvariant *cf_htmleditformat(const cfvariant *str, const cfvariant *version = nullptr);
cfvariant *cf_iif(const cfvariant *condition, const cfvariant *expr1, const cfvariant *expr2,
                  string &out, void *cgi, void *server, void *cookie,
                  void *application, void *session, void *url, void *form,
                  void *variables);
cfvariant *cf_imageclearrect(const cfvariant *image, const cfvariant *x, const cfvariant *y,
                             const cfvariant *width, const cfvariant *height);
cfvariant *cf_imagecreatecaptcha(const cfvariant *height, const cfvariant *width, const cfvariant *text,
                                 const cfvariant *difficulty, const cfvariant *font, const cfvariant *fontsize);
cfvariant *cf_imagedrawarc(const cfvariant *image, const cfvariant *x, const cfvariant *y,
                           const cfvariant *width, const cfvariant *height, const cfvariant *startAngle,
                           const cfvariant *arcAngle, const cfvariant *filled);
cfvariant *cf_imagedrawbeveledrect(const cfvariant *image, const cfvariant *x, const cfvariant *y,
                                   const cfvariant *width, const cfvariant *height, const cfvariant *raised,
                                   const cfvariant *filled);
cfvariant *cf_imagedrawcubiccurve(const cfvariant *image, const cfvariant *ctrlx1, const cfvariant *ctrly1,
                                  const cfvariant *ctrlx2, const cfvariant *ctrly2, const cfvariant *x1,
                                  const cfvariant *y1, const cfvariant *x2, const cfvariant *y2);
cfvariant *cf_imagedrawline(const cfvariant *image, const cfvariant *x1, const cfvariant *y1,
                            const cfvariant *x2, const cfvariant *y2);
cfvariant *cf_imagedrawlines(const cfvariant *image, const cfvariant *xcords, const cfvariant *ycords,
                             const cfvariant *isPolygon, const cfvariant *filled);
cfvariant *cf_imagedrawoval(const cfvariant *image, const cfvariant *x, const cfvariant *y,
                            const cfvariant *width, const cfvariant *height, const cfvariant *filled);
cfvariant *cf_imagedrawpoint(const cfvariant *image, const cfvariant *x, const cfvariant *y);
cfvariant *cf_imagedrawquadraticcurve(const cfvariant *image, const cfvariant *x1, const cfvariant *y1,
                                      const cfvariant *ctrlx1, const cfvariant *ctrly1, const cfvariant *x2,
                                      const cfvariant *y2);
cfvariant *cf_imagedrawrect(const cfvariant *image, const cfvariant *x, const cfvariant *y,
                            const cfvariant *width, const cfvariant *height, const cfvariant *filled);
cfvariant *cf_imagedrawroundrect(const cfvariant *image, const cfvariant *x, const cfvariant *y,
                                 const cfvariant *width, const cfvariant *height, const cfvariant *arcwidth,
                                 const cfvariant *archeight, const cfvariant *filled);
cfvariant *cf_imagedrawtext(const cfvariant *image, const cfvariant *str, const cfvariant *x,
                             const cfvariant *y, const cfvariant *attributes);
cfvariant *cf_imagegetblob(const cfvariant *image);
cfvariant *cf_imagegetbufferedimage(const cfvariant *name);
cfvariant *cf_imagegetexifmetadata(const cfvariant *name);
cfvariant *cf_imagegetexiftag(const cfvariant *name, const cfvariant *tagname);
cfvariant *cf_imagegetheight(const cfvariant *image);
cfvariant *cf_imagegetiptcmetadata(const cfvariant *name);
cfvariant *cf_imagegetiptctag(const cfvariant *name, const cfvariant *tagname);
cfvariant *cf_imagegetmetadata(const cfvariant *image);
cfvariant *cf_imagegetwidth(const cfvariant *image);
cfvariant *cf_imageinfo(const cfvariant *image);
cfvariant *cf_imagenew(const cfvariant *source, const cfvariant *width, const cfvariant *height,
                        const cfvariant *imageType, const cfvariant *canvasColor);
cfvariant *cf_imageread(const cfvariant *path);
cfvariant *cf_imagereadbase64(const cfvariant *data);
cfvariant *cf_imagerotatedrawingaxis(const cfvariant *image, const cfvariant *angle, const cfvariant *x,
                                      const cfvariant *y);
cfvariant *cf_imagesetantialiasing(const cfvariant *image, const cfvariant *antialias);
cfvariant *cf_imagesetbackgroundcolor(const cfvariant *image, const cfvariant *color);
cfvariant *cf_imagesetdrawingcolor(const cfvariant *image, const cfvariant *color);
cfvariant *cf_imagesetdrawingstroke(const cfvariant *image, const cfvariant *attributes);
cfvariant *cf_imagesetdrawingtransparency(const cfvariant *image, const cfvariant *percent);
cfvariant *cf_imagesheardrawingaxis(const cfvariant *image, const cfvariant *shrx, const cfvariant *shry);
cfvariant *cf_imagetranslatedrawingaxis(const cfvariant *image, const cfvariant *x, const cfvariant *y);
cfvariant *cf_imagewrite(const cfvariant *image, const cfvariant *destination, const cfvariant *quality, const cfvariant *overwrite);
cfvariant *cf_imagewritebase64(const cfvariant *image, const cfvariant *destination, const cfvariant *format,
                               const cfvariant *inHTMLFormat, const cfvariant *overwrite);
cfvariant *cf_imagexordrawingmode(const cfvariant *image, const cfvariant *color);
cfvariant *cf_initsamlauthrequest();
cfvariant *cf_initsamllogoutrequest();
cfvariant *cf_inputbasen();
cfvariant *cf_insert(const cfvariant *sub, const cfvariant *str, const cfvariant *pos);
cfvariant *cf_interruptthread();
cfvariant *cf_invalidateoauthaccesstoken();
cfvariant *cf_invoke(const cfvariant *object, const cfvariant *methodName, const cfvariant *arguments,
                     string &out, void *cgi, void *server, void *cookie, void *application,
                     void *session, void *url, void *form, void *variables);
// InvokeCFClientFunction is not a ColdFusion 2025 function: calling it fails
// with "Variable INVOKECFCLIENTFUNCTION is undefined." (verified on the RDS
// host). The stub reproduces that by throwing a variable-undefined error.
cfvariant *cf_invokecfclientfunction(const cfvariant *arg);
cfvariant *cf_isauthenticated();
cfvariant *cf_isauthorized();
cfvariant *cf_isbinary(const cfvariant *val);
cfvariant *cf_isboolean(const cfvariant *val);
cfvariant *cf_isclosure(const cfvariant *val);
cfvariant *cf_iscustomfunction(const cfvariant *val);
cfvariant *cf_isdateobject(const cfvariant *value);
cfvariant *cf_isddx(const cfvariant *value);
cfvariant *cf_isdebugmode();
cfvariant *cf_isdefined(const cfvariant *a1, void *cgi, void *server, void *cookie,
                        void *application, void *session, void *url, void *form,
                        void *variables);
cfvariant *cf_isfileobject(const cfvariant *val);
cfvariant *cf_isimage(const cfvariant *val);
cfvariant *cf_isimagefile(const cfvariant *value, const cfvariant *format);
cfvariant *cf_isinstanceof(const cfvariant *obj, const cfvariant *typeName);
cfvariant *cf_isipv6(const cfvariant *value);
cfvariant *cf_isjson(const cfvariant *arg);
cfvariant *cf_isk2serverabroker();
cfvariant *cf_isk2serverdoccountexceeded();
cfvariant *cf_isk2serveronline();
cfvariant *cf_isleapyear(const cfvariant *year);
cfvariant *cf_islocalhost(const cfvariant *value);
cfvariant *cf_isnull(const cfvariant *val);
cfvariant *cf_isnumeric(const cfvariant *val);
cfvariant *cf_isnumericdate(const cfvariant *value);
cfvariant *cf_isobject(const cfvariant *val);
cfvariant *cf_isonline(const cfvariant *value);
cfvariant *cf_ispdfarchive(const cfvariant *value, const cfvariant *standard = nullptr);
cfvariant *cf_ispdffile(const cfvariant *value);
cfvariant *cf_ispdfobject(const cfvariant *value);
cfvariant *cf_isprotected();
cfvariant *cf_isquery(const cfvariant *val);
cfvariant *cf_issafehtml();
cfvariant *cf_issamllogoutresponse();
cfvariant *cf_issimplevalue(const cfvariant *val);
cfvariant *cf_issoaprequest();
cfvariant *cf_isspreadsheetfile();
cfvariant *cf_isspreadsheetobject();
cfvariant *cf_isstruct(const cfvariant *val);
cfvariant *cf_isthreadinterrupted(const cfvariant *threadName);
cfvariant *cf_isvalid(const cfvariant *type, const cfvariant *value, const cfvariant *min = nullptr, const cfvariant *max = nullptr, const cfvariant *pattern = nullptr);
cfvariant *cf_isvalidoauthaccesstoken();
cfvariant *cf_iswddx(const cfvariant *value);
cfvariant *cf_isxml(const cfvariant *arg);
cfvariant *cf_isxmlattribute(const cfvariant *arg);
cfvariant *cf_isxmldoc(const cfvariant *arg);
cfvariant *cf_isxmlelem(const cfvariant *arg);
cfvariant *cf_isxmlnode(const cfvariant *arg);
cfvariant *cf_isxmlroot(const cfvariant *arg);
cfvariant *cf_javacast();
cfvariant *cf_jsstringformat();
cfvariant *cf_listeach(const cfvariant *list, const cfvariant *callback, const cfvariant *delim, const cfvariant *includeEmptyFields,
                       string &out, void *cgi, void *server, void *cookie, void *application, void *session, void *url, void *form, void *variables);
cfvariant *cf_listfilter(const cfvariant *list, const cfvariant *callback, const cfvariant *delim, const cfvariant *includeEmptyFields,
                         string &out, void *cgi, void *server, void *cookie, void *application, void *session, void *url, void *form, void *variables);
cfvariant *cf_listgetduplicates(const cfvariant *list, const cfvariant *delim);
cfvariant *cf_listmap(const cfvariant *list, const cfvariant *callback, const cfvariant *delim, const cfvariant *includeEmptyFields,
                      string &out, void *cgi, void *server, void *cookie, void *application, void *session, void *url, void *form, void *variables);
cfvariant *cf_listqualify(const cfvariant *list, const cfvariant *qualifier, const cfvariant *delimiters,
                          const cfvariant *elements, const cfvariant *includeEmptyValues);
cfvariant *cf_listreduce(const cfvariant *list, const cfvariant *callback, const cfvariant *initialValue,
                         const cfvariant *delim, const cfvariant *includeEmptyFields,
                         string &out, void *cgi, void *server, void *cookie, void *application, void *session, void *url, void *form, void *variables);
cfvariant *cf_listremoveduplicates(const cfvariant *list, const cfvariant *delim, const cfvariant *ignoreCase);
cfvariant *cf_listsort(const cfvariant *list, const cfvariant *sortType, const cfvariant *sortOrder,
                       const cfvariant *delimiters, const cfvariant *includeEmptyFields, const cfvariant *localeSensitive,
                       string &out, void *cgi, void *server, void *cookie, void *application, void *session, void *url, void *form, void *variables);
cfvariant *cf_listvaluecount(const cfvariant *list, const cfvariant *value, const cfvariant *delim);
cfvariant *cf_listvaluecountnocase(const cfvariant *list, const cfvariant *value, const cfvariant *delim);
cfvariant *cf_ljustify(const cfvariant *str, const cfvariant *length);
cfvariant *cf_location(const cfvariant *url, const cfvariant *addtoken,
                       const cfvariant *statuscode, int argCount);
cfvariant *cf_lscurrencyformat(const cfvariant *number, const cfvariant *type = nullptr, const cfvariant *locale = nullptr);
cfvariant *cf_lsdateformat(const cfvariant *date, const cfvariant *mask = nullptr, const cfvariant *locale = nullptr);
cfvariant *cf_lsdatetimeformat(const cfvariant *date, const cfvariant *mask = nullptr, const cfvariant *locale = nullptr);
cfvariant *cf_lseurocurrencyformat(const cfvariant *number, const cfvariant *type = nullptr, const cfvariant *locale = nullptr);
cfvariant *cf_lsiscurrency(const cfvariant *string, const cfvariant *locale = nullptr);
cfvariant *cf_lsisdate(const cfvariant *string, const cfvariant *locale = nullptr);
cfvariant *cf_lsisnumeric(const cfvariant *val);
cfvariant *cf_lsnumberformat(const cfvariant *number, const cfvariant *mask = nullptr, const cfvariant *locale = nullptr);
cfvariant *cf_lsparsecurrency(const cfvariant *string, const cfvariant *locale = nullptr);
cfvariant *cf_lsparsedatetime(const cfvariant *string, const cfvariant *locale = nullptr);
cfvariant *cf_lsparseeurocurrency(const cfvariant *string, const cfvariant *locale = nullptr);
cfvariant *cf_lsparsenumber(const cfvariant *string, const cfvariant *locale = nullptr);
cfvariant *cf_lstimeformat(const cfvariant *time, const cfvariant *mask = nullptr, const cfvariant *locale = nullptr);
cfvariant *cf_getlocale();
cfvariant *cf_setlocale(const cfvariant *locale);
cfvariant *cf_monthasstring(const cfvariant *month_number, const cfvariant *locale = nullptr);
cfvariant *cf_numberformat(const cfvariant *number, const cfvariant *mask = nullptr);
cfvariant *cf_objectequals(const cfvariant *clientobject, const cfvariant *originalobject);
cfvariant *cf_objectload(const cfvariant *binaryOrFile);
cfvariant *cf_objectsave(const cfvariant *obj, const cfvariant *file = nullptr);
cfvariant *cf_onwsauthenticate();
cfvariant *cf_ormclearsession();
cfvariant *cf_ormcloseallsessions();
cfvariant *cf_ormclosesession();
cfvariant *cf_ormevictcollection();
cfvariant *cf_ormevictentity();
cfvariant *cf_ormevictqueries();
cfvariant *cf_ormexecutequery();
cfvariant *cf_ormflush();
cfvariant *cf_ormflushall();
cfvariant *cf_ormgetsession();
cfvariant *cf_ormgetsessionfactory();
cfvariant *cf_ormindex();
cfvariant *cf_ormindexpurge();
cfvariant *cf_ormreload();
cfvariant *cf_ormsearch();
cfvariant *cf_ormsearchoffline();
cfvariant *cf_parsedatetime(const cfvariant *date_string, const cfvariant *popup = nullptr);
cfvariant *cf_precisionevaluate(const cfvariant *expr,
                                void *cgi, void *server, void *cookie, void *application,
                                void *session, void *url, void *form, void *variables);
cfvariant *cf_preservesinglequotes(const cfvariant *variable);
cfvariant *cf_processsamllogoutrequest();
cfvariant *cf_processsamlresponse();
cfvariant *cf_quarter(const cfvariant *date);
cfvariant *cf_queryaddcolumn(cfvariant *query, const cfvariant *column_name, const cfvariant *datatype_or_array, const cfvariant *array_name);
cfvariant *cf_queryaddrow(cfvariant *query, const cfvariant *rows);
cfvariant *cf_queryconvertforgrid(const cfvariant *query, const cfvariant *page, const cfvariant *pageSize);
cfvariant *cf_queryeach(const cfvariant *query, const cfvariant *callback, const cfvariant *parallel, const cfvariant *maxThreads,
                        string &out, void *cgi, void *server, void *cookie, void *application, void *session, void *url, void *form, void *variables);
cfvariant *cf_run_query(const std::string &sql, const cfvariant *attrs,
                        void *cgi, void *server, void *cookie, void *application,
                        void *session, void *url, void *form, void *variables);

// Query-cache helpers (cfquery cachedwithin/cacheid, RemoveCachedQuery). The
// query cache stores each result as a type-preserving JSON blob in the
// CacheStore's "QUERY" region. `cf_query_cache_key` computes the cache id from
// the SQL + datasource + params (or returns the explicit cacheid), mirroring
// CF's QueryDetails hash / RemoveCachedQuery key.
std::string cf_query_cache_key(const std::string &sql, const std::string &datasource,
                               const cfvariant *params, const cfvariant *cacheid);
cfvariant *cf_query_cache_serialize(const cfvariant *query);
cfvariant *cf_query_cache_deserialize(const cfvariant *json);
cfvariant *cf_queryexecute(const cfvariant *sql, const cfvariant *params, const cfvariant *options,
                           void *cgi, void *server, void *cookie, void *application,
                           void *session, void *url, void *form, void *variables);
cfvariant *cf_queryfilter(const cfvariant *query, const cfvariant *callback, const cfvariant *parallel, const cfvariant *maxThreads,
                          string &out, void *cgi, void *server, void *cookie, void *application, void *session, void *url, void *form, void *variables);
cfvariant *cf_querygetresult(const cfvariant *query);
cfvariant *cf_querygetrow(const cfvariant *query, const cfvariant *row_number);
cfvariant *cf_querykeyexists(const cfvariant *query, const cfvariant *key);
cfvariant *cf_querymap(const cfvariant *query, const cfvariant *callback, const cfvariant *parallel, const cfvariant *maxThreads,
                       string &out, void *cgi, void *server, void *cookie, void *application, void *session, void *url, void *form, void *variables);
cfvariant *cf_querynew(const cfvariant *columnList, const cfvariant *columnTypeList, const cfvariant *rowData);
cfvariant *cf_queryreduce(const cfvariant *query, const cfvariant *callback, const cfvariant *initialValue,
                          string &out, void *cgi, void *server, void *cookie, void *application, void *session, void *url, void *form, void *variables);
cfvariant *cf_querysetcell(cfvariant *query, const cfvariant *column, const cfvariant *value, const cfvariant *row);
cfvariant *cf_quotedvaluelist(const cfvariant *column, const cfvariant *delimiter);
cfvariant *cf_reescape(const cfvariant *str);
cfvariant *cf_refind(const cfvariant *regex, const cfvariant *str, const cfvariant *start, const cfvariant *returnsub, const cfvariant *scope);
cfvariant *cf_refindnocase(const cfvariant *regex, const cfvariant *str, const cfvariant *start, const cfvariant *returnsub, const cfvariant *scope);
cfvariant *cf_releasecomobject();
cfvariant *cf_rematch(const cfvariant *regex, const cfvariant *str);
cfvariant *cf_rematchnocase(const cfvariant *regex, const cfvariant *str);
cfvariant *cf_replacelist(const cfvariant *str, const cfvariant *list1, const cfvariant *list2,
                          const cfvariant *delim1 = nullptr, const cfvariant *delim2 = nullptr,
                          const cfvariant *includeEmptyFields = nullptr);
cfvariant *cf_rereplace(const cfvariant *str, const cfvariant *regex, const cfvariant *sub, const cfvariant *scope);
cfvariant *cf_rereplacenocase(const cfvariant *str, const cfvariant *regex, const cfvariant *sub, const cfvariant *scope);
cfvariant *cf_restdeleteapplication();
cfvariant *cf_restinitapplication();
cfvariant *cf_restsetresponse();
cfvariant *cf_rjustify(const cfvariant *str, const cfvariant *length);
cfvariant *cf_sendgatewaymessage();
cfvariant *cf_sendsamllogoutresponse();
cfvariant *cf_serialize(const cfvariant *data, const cfvariant *type);
cfvariant *cf_serializejson(const cfvariant *data, const cfvariant *queryFormat = nullptr,
                            const cfvariant *useSecureJSONPrefix = nullptr,
                            const cfvariant *useCustomSerializer = nullptr);
cfvariant *cf_serializexml(const cfvariant *arg);
cfvariant *cf_sessiongetmetadata();
cfvariant *cf_sessioninvalidate();
cfvariant *cf_sessionrotate();
cfvariant *cf_setday(const cfvariant *date, const cfvariant *day);
cfvariant *cf_setencoding();
cfvariant *cf_sethour(const cfvariant *date, const cfvariant *hour);
cfvariant *cf_setlocale(const cfvariant *locale);
cfvariant *cf_setminute(const cfvariant *date, const cfvariant *minute);
cfvariant *cf_setmonth(const cfvariant *date, const cfvariant *month);
cfvariant *cf_setprofilestring(const cfvariant *path, const cfvariant *section, const cfvariant *entry, const cfvariant *value, const cfvariant *encoding = nullptr);
cfvariant *cf_setpropertystring(const cfvariant *filePath, const cfvariant *keyOrMap, const cfvariant *value = nullptr, const cfvariant *encoding = nullptr);
cfvariant *cf_setsecond(const cfvariant *date, const cfvariant *second);
cfvariant *cf_setvariable(const cfvariant *name, const cfvariant *value,
                          void *cgi, void *server, void *cookie,
                          void *application, void *session, void *url,
                          void *form, void *variables);
cfvariant *cf_setyear(const cfvariant *date, const cfvariant *year);
cfvariant *cf_week(const cfvariant *date);
cfvariant *cf_sleep(const cfvariant *duration);
cfvariant *cf_spanexcluding();
cfvariant *cf_spanincluding();
cfvariant *cf_spreadsheetaddautofilter();
cfvariant *cf_spreadsheetaddcolumn();
cfvariant *cf_spreadsheetaddfreezepane();
cfvariant *cf_spreadsheetaddimage();
cfvariant *cf_spreadsheetaddinfo();
cfvariant *cf_spreadsheetaddpagebreaks();
cfvariant *cf_spreadsheetaddprintgridlines();
cfvariant *cf_spreadsheetaddrow();
cfvariant *cf_spreadsheetaddrows();
cfvariant *cf_spreadsheetaddsplitpane();
cfvariant *cf_spreadsheetcreatesheet();
cfvariant *cf_spreadsheetdeletecolumn();
cfvariant *cf_spreadsheetdeletecolumns();
cfvariant *cf_spreadsheetdeleterow();
cfvariant *cf_spreadsheetdeleterows();
cfvariant *cf_spreadsheetformatcell();
cfvariant *cf_spreadsheetformatcellrange();
cfvariant *cf_spreadsheetformatcolumn();
cfvariant *cf_spreadsheetformatcolumns();
cfvariant *cf_spreadsheetformatrow();
cfvariant *cf_spreadsheetformatrows();
cfvariant *cf_spreadsheetgetcellcomment();
cfvariant *cf_spreadsheetgetcellformula();
cfvariant *cf_spreadsheetgetcellvalue();
cfvariant *cf_spreadsheetgetcolumncount();
cfvariant *cf_spreadsheetgetcolumnwidth();
cfvariant *cf_spreadsheetgetlastrownumber();
cfvariant *cf_spreadsheetgetprintorientation();
cfvariant *cf_spreadsheetgroupcolumns();
cfvariant *cf_spreadsheetgrouprows();
cfvariant *cf_spreadsheetinfo();
cfvariant *cf_spreadsheetisbinaryformat();
cfvariant *cf_spreadsheetiscolumnhidden();
cfvariant *cf_spreadsheetisrowhidden();
cfvariant *cf_spreadsheetisstreamingxmlformat();
cfvariant *cf_spreadsheetisxmlformat();
cfvariant *cf_spreadsheetmergecells();
cfvariant *cf_spreadsheetnew();
cfvariant *cf_spreadsheetread();
cfvariant *cf_spreadsheetreadbinary();
cfvariant *cf_spreadsheetremovecolumnbreak();
cfvariant *cf_spreadsheetremoveprintgridlines();
cfvariant *cf_spreadsheetremoverowbreak();
cfvariant *cf_spreadsheetremovesheet();
cfvariant *cf_spreadsheetremovesheetnumber();
cfvariant *cf_spreadsheetrenamesheet();
cfvariant *cf_spreadsheetsetactivesheet();
cfvariant *cf_spreadsheetsetactivesheetnumber();
cfvariant *cf_spreadsheetsetcellcomment();
cfvariant *cf_spreadsheetsetcellformula();
cfvariant *cf_spreadsheetsetcellvalue();
cfvariant *cf_spreadsheetsetcolumnbreak();
cfvariant *cf_spreadsheetsetcolumnhidden();
cfvariant *cf_spreadsheetsetcolumnwidth();
cfvariant *cf_spreadsheetsetfittopage();
cfvariant *cf_spreadsheetsetfooter();
cfvariant *cf_spreadsheetsetfooterimage();
cfvariant *cf_spreadsheetsetheader();
cfvariant *cf_spreadsheetsetheaderimage();
cfvariant *cf_spreadsheetsetrowbreak();
cfvariant *cf_spreadsheetsetrowheight();
cfvariant *cf_spreadsheetsetrowhidden();
cfvariant *cf_spreadsheetshiftcolumns();
cfvariant *cf_spreadsheetshiftrows();
cfvariant *cf_spreadsheetungroupcolumns();
cfvariant *cf_spreadsheetungrouprows();
cfvariant *cf_spreadsheetwrite();
cfvariant *cf_storeaddacl();
cfvariant *cf_storegetacl();
cfvariant *cf_storegetmetadata();
cfvariant *cf_storesetacl();
cfvariant *cf_storesetmetadata();
cfvariant *cf_streamingspreadsheetcleanup();
cfvariant *cf_streamingspreadsheetisstreamingxmlformat();
cfvariant *cf_streamingspreadsheetisxmlformat();
cfvariant *cf_streamingspreadsheetnew();
cfvariant *cf_streamingspreadsheetprocess();
cfvariant *cf_streamingspreadsheetread();
cfvariant *cf_stripcr();
cfvariant *cf_structappend(cfvariant *dest, const cfvariant *source, const cfvariant *overwriteFlag);
cfvariant *cf_structcopy(const cfvariant *st);
cfvariant *cf_structeach(const cfvariant *st, const cfvariant *callback, const cfvariant *parallel, const cfvariant *maxThreads,
                         string &out, void *cgi, void *server, void *cookie, void *application, void *session, void *url, void *form, void *variables);
cfvariant *cf_structfilter(const cfvariant *st, const cfvariant *callback, const cfvariant *parallel, const cfvariant *maxThreads,
                           string &out, void *cgi, void *server, void *cookie, void *application, void *session, void *url, void *form, void *variables);
cfvariant *cf_structfindkey(const cfvariant *top, const cfvariant *value, const cfvariant *scope);
cfvariant *cf_structfindvalue(const cfvariant *top, const cfvariant *value, const cfvariant *scope);
cfvariant *cf_structget(const cfvariant *path, void *variables);
cfvariant *cf_structgetmetadata(const cfvariant *st);
cfvariant *cf_structmap(const cfvariant *st, const cfvariant *callback, const cfvariant *parallel, const cfvariant *maxThreads,
                        string &out, void *cgi, void *server, void *cookie, void *application, void *session, void *url, void *form, void *variables);
cfvariant *cf_structreduce(const cfvariant *st, const cfvariant *callback, const cfvariant *initialValue,
                           string &out, void *cgi, void *server, void *cookie, void *application, void *session, void *url, void *form, void *variables);
cfvariant *cf_structsetmetadata(cfvariant *st, const cfvariant *meta);
cfvariant *cf_structsort(const cfvariant *st, const cfvariant *sortType, const cfvariant *sortOrder,
                         const cfvariant *path, const cfvariant *localeSensitive,
                         string &out, void *cgi, void *server, void *cookie, void *application, void *session, void *url, void *form, void *variables);
cfvariant *cf_structtosorted(const cfvariant *st, const cfvariant *sortType, const cfvariant *sortOrder,
                             const cfvariant *localeSensitive,
                             string &out, void *cgi, void *server, void *cookie, void *application, void *session, void *url, void *form, void *variables);
cfvariant *cf_threadjoin();
cfvariant *cf_threadterminate();
cfvariant *cf_throw();
cfvariant *cf_tobase64(const cfvariant *input, const cfvariant *encoding);
cfvariant *cf_tobinary(const cfvariant *input);
cfvariant *cf_toscript(const cfvariant *cfvar, const cfvariant *javascriptvar, const cfvariant *outputformat, const cfvariant *asformat);
cfvariant *cf_tostring(const cfvariant *value, const cfvariant *encoding);
cfvariant *cf_trace(const cfvariant *namedArgs);
cfvariant *cf_trace_tag(const cfvariant *text, const cfvariant *type,
                        const cfvariant *category, const cfvariant *inlineValue,
                        const cfvariant *abortValue, const cfvariant *var);
cfvariant *cf_transactioncommit();
cfvariant *cf_transactionrollback(const cfvariant *savepoint);
cfvariant *cf_transactionsetsavepoint(const cfvariant *savepoint);
cfvariant *cf_urldecode(const cfvariant *str, const cfvariant *charset);
cfvariant *cf_urlencodedformat(const cfvariant *str, const cfvariant *charset);
cfvariant *cf_urlsessionformat(const cfvariant *url);
cfvariant *cf_val(const cfvariant *str);
cfvariant *cf_valuelist(const cfvariant *column, const cfvariant *delimiter);
cfvariant *cf_verifyclient();
cfvariant *cf_wrap(const cfvariant *str, const cfvariant *limit, const cfvariant *strip = nullptr);
cfvariant *cf_writedump(const cfvariant *var, const cfvariant *output, const cfvariant *format, const cfvariant *abort, const cfvariant *label, const cfvariant *metainfo, const cfvariant *top, const cfvariant *show, const cfvariant *hide, const cfvariant *keys, const cfvariant *expand, const cfvariant *showUDFs);
cfvariant *cf_writelog(const cfvariant *text, const cfvariant *type, const cfvariant *application, const cfvariant *file, const cfvariant *log);

// <cftimer> runtime (src/cftags/tag_logging.cpp). cf_timer_begin validates the
// `type` attribute (a catchable Template error for an invalid value, CF's
// IllegalSwitchValueException) and returns the start timestamp; cf_timer_end
// measures the elapsed time and, when config::debugEnabled is set, emits the
// timing per type (inline/comment/outline). With debugging disabled (the
// engine's default) nothing is written and the tag only evaluates its body.
int64_t cf_timer_begin(const cfvariant *type);
void cf_timer_end(int64_t start, const cfvariant *type, const cfvariant *label, void *out);
void cf_emit_writedump(void *out, cfvariant *dumpResult);
cfvariant *cf_wsgetallchannels();
cfvariant *cf_wsgetsubscribers();
cfvariant *cf_wspublish();
cfvariant *cf_wssendmessage();
cfvariant *cf_xmlchildpos(const cfvariant *arg0, const cfvariant *arg1, const cfvariant *arg2);cfvariant *cf_xmlelemnew(const cfvariant *arg0, const cfvariant *arg1, const cfvariant *arg2 = nullptr);
cfvariant *cf_xmlformat(const cfvariant *arg0, const cfvariant *arg1 = nullptr);
cfvariant *cf_xmlgetnodetype(const cfvariant *arg);
cfvariant *cf_xmlnew(const cfvariant *arg0 = nullptr);
cfvariant *cf_xmlparse(const cfvariant *arg0, const cfvariant *arg1 = nullptr, const cfvariant *arg2 = nullptr);
cfvariant *cf_xmlsearch(const cfvariant *arg0, const cfvariant *arg1, const cfvariant *arg2 = nullptr);
cfvariant *cf_xmltransform(const cfvariant *arg0, const cfvariant *arg1, const cfvariant *arg2 = nullptr);
cfvariant *cf_xmlvalidate(const cfvariant *arg0, const cfvariant *arg1 = nullptr);

} // namespace cfml
