/**
 * @file tag_cferror.cpp
 * @brief <cferror> runtime (register + exception dispatch).
 *
 * Mirrors Adobe ColdFusion 2025 (update 11) semantics, verified live against a
 * CF 2025 server and decoded from the update jar's ErrorTag, FusionContext and
 * ExceptionFilter:
 *
 *   - ErrorTag.doStartTag: the template attribute is resolved FIRST (page
 *     directory joined with the raw value, then the web root for absolute
 *     values) and must exist; the type attribute then dispatches to
 *     validation / exception / request handlers, and any other value throws
 *     InvalidTagAttributeException.
 *   - FusionContext.addExceptionHandler: built-in exception names register
 *     UNNAMED handlers for the mapped class (expression -> RuntimeException,
 *     any -> java.lang.Throwable root, security -> AccessControlException,
 *     ...); any other name registers a named CustomException handler. Custom
 *     handlers replace an existing custom handler only when both are unnamed
 *     or their names match case-insensitively, and always sort AFTER the
 *     built-ins; built-in handlers replace on exact class equality, insert
 *     before the first handler whose class is an ancestor of the new class
 *     (most-specific-first), and otherwise append.
 *   - ExceptionFilter.runExceptionHandler: a handler matches when its class is
 *     an ancestor of the thrown exception's class. Custom exceptions skip
 *     every unnamed handler (exception == null), match a named handler only on
 *     exact case-insensitive type name, and remember the "any" (Throwable)
 *     handler as a fallback that runs only when nothing else matched. The
 *     handler template runs as an include with a fresh writer (the buffered
 *     output becomes error.generatedcontent) and the error/cferror structs as
 *     variables. A non-abort exception thrown by the handler template falls
 *     through to the request-type handler with that NEW exception; an abort is
 *     swallowed.
 *   - ExceptionFilter.handleException: missing templates (TemplateNotFound-
 *     Exception) never reach <cferror>; the dispatch happens once per request
 *     (the "biscuit" marker); a successful request-type handler sets HTTP 500.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>
#include <webstrada/exceptions.h>
#include <webstrada/string.h>

#include <sys/stat.h>
#include <algorithm>
#include <chrono>
#include <climits>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace {

using webstrada::string;
using webstrada::cfvariant;

// A registered exception handler (CF's FusionContext$ExceptionHandler).
struct CferrorHandler {
    std::string templatePath;    // resolved absolute path ("" when unset)
    std::string mailto;
    std::string exceptionClass;  // engine class name (see cf_eh_class_distance)
    std::string exceptionName;   // custom handler name ("" when unnamed)
};

// Per-request <cferror> registry (CF's FusionContext exceptionHandlers /
// requestExceptionHandler / validationExceptionHandler plus the
// ExceptionFilter once-per-request "biscuit" marker).
struct CferrorRuntime {
    std::vector<CferrorHandler> exceptionHandlers;
    CferrorHandler requestHandler;    // templatePath empty when unset
    CferrorHandler validationHandler; // templatePath empty when unset
    bool invoked = false;
};

thread_local CferrorRuntime *g_cferrorRuntime = nullptr;

CferrorRuntime *cferrorRuntime()
{
    if (!g_cferrorRuntime) {
        g_cferrorRuntime = new CferrorRuntime();
    }
    return g_cferrorRuntime;
}

bool fileExists(const char *path)
{
    struct stat st;
    return stat(path, &st) == 0 && S_ISREG(st.st_mode);
}

std::string lowercase(const std::string &s)
{
    std::string r = s;
    std::transform(r.begin(), r.end(), r.begin(),
                   [](unsigned char c) { return (char)std::tolower(c); });
    return r;
}

// The exception class a <cferror exception="..."> handler registers for,
// mirroring ErrorTag.doStartTag's class table (expression -> RuntimeException,
// any -> java.lang.Throwable, security -> AccessControlException, ...).
// Returns "" for an unrecognized name (a custom-type handler).
const char *exceptionClassForAttr(const std::string &excLow)
{
    if (excLow == "application") return "ApplicationException";
    if (excLow == "database") return "DatabaseException";
    if (excLow == "lock") return "LockException";
    if (excLow == "object") return "ObjectException";
    if (excLow == "missinginclude") return "MissingIncludeException";
    if (excLow == "template") return "TemplateException";
    if (excLow == "security") return "AccessControlException";
    if (excLow == "expression") return "java.lang.RuntimeException";
    if (excLow == "any") return "java.lang.Throwable";
    return nullptr;
}

// Resolve the template attribute like ErrorTag.doStartTag: first the current
// page's directory joined with the raw value, then (for absolute-looking
// values) the web root. Throws CF's InvalidErrorTemplateException message when
// neither exists.
std::string resolveErrorTemplate(const cfml::IncludeRuntime *rt, const std::string &tpl)
{
    std::string dir;
    if (rt && !rt->currentPath.empty()) {
        size_t slash = rt->currentPath.find_last_of('/');
        if (slash != std::string::npos) {
            // CF: pagePath.substring(0, lastIndex+1) keeps the separator.
            dir = rt->currentPath.substr(0, slash + 1);
        }
    }
    std::string candidate1 = dir + tpl;
    if (!candidate1.empty() && fileExists(candidate1.c_str())) {
        return candidate1;
    }
    if (!tpl.empty() && (tpl[0] == '/' || tpl[0] == '\\')) {
        std::string candidate2 = (rt ? rt->webRoot : std::string()) + tpl;
        if (fileExists(candidate2.c_str())) {
            return candidate2;
        }
    }
    throw webstrada::exception("MissingInclude",
        ("Error attempting to resolve the template " + tpl + ".").c_str(),
        "The template could not be found.");
}

// CF's FusionContext.addExceptionHandler insertion algorithm.
static void addExceptionHandler(CferrorRuntime *reg, const CferrorHandler &h)
{
    const bool isCustom = (h.exceptionClass == "CustomException");
    for (size_t i = 0; i < reg->exceptionHandlers.size(); i++) {
        CferrorHandler &existing = reg->exceptionHandlers[i];
        if (isCustom) {
            if (existing.exceptionClass == "CustomException") {
                // Replace only when both unnamed or the names match
                // case-insensitively (CF's equalsIgnoreCase on exception).
                if ((existing.exceptionName.empty() && h.exceptionName.empty()) ||
                    (!existing.exceptionName.empty() && !h.exceptionName.empty() &&
                     lowercase(existing.exceptionName) == lowercase(h.exceptionName))) {
                    reg->exceptionHandlers[i] = h;
                    return;
                }
            }
        } else {
            if (existing.exceptionClass == h.exceptionClass) {
                reg->exceptionHandlers[i] = h;
                return;
            }
            // existing.type.isAssignableFrom(newType): insert before the first
            // handler whose class is an ancestor of the new handler's class.
            if (cfml::cf_eh_class_distance(h.exceptionClass.c_str(),
                                           existing.exceptionClass.c_str()) != INT_MAX) {
                reg->exceptionHandlers.insert(reg->exceptionHandlers.begin() + i, h);
                return;
            }
        }
    }
    reg->exceptionHandlers.push_back(h);
}

// CF's runExceptionHandler matching: the first handler whose class is an
// ancestor of the thrown exception's class wins; a custom exception skips
// every unnamed handler, matches a named handler on exact case-insensitive
// type name, and remembers the "any" (root-class) handler as the fallback
// (returned through `anyHandler`) which runs only when nothing matched.
static CferrorHandler *matchExceptionHandler(CferrorRuntime *reg,
                                             const webstrada::exception &ex,
                                             CferrorHandler **anyHandler)
{
    *anyHandler = nullptr;
    const char *cls = ex.m_isCustom ? "CustomException" : cfml::cf_eh_thrown_class(ex.m_type);
    for (auto &h : reg->exceptionHandlers) {
        if (cfml::cf_eh_class_distance(cls, h.exceptionClass.c_str()) == INT_MAX) {
            continue;
        }
        if (ex.m_isCustom) {
            if (h.exceptionClass == "java.lang.Throwable") {
                *anyHandler = &h;
            }
            if (h.exceptionName.empty()) {
                continue;  // CF: handler.exception == null -> skip
            }
            if (string(h.exceptionName.c_str(), h.exceptionName.size()).compareCaseInsensitive(ex.m_type) != 0) {
                continue;
            }
        }
        return &h;
    }
    return nullptr;
}

// CF's ExceptionScope.get on the ROOTCAUSE throwable: the type is the custom
// type name for user throws, the engine type otherwise. The wrapper struct
// (CfErrorWrapper) always reports TYPE "coldfusion.runtime.CfErrorWrapper"
// and DETAIL "" (the wrapper is not a NeoException, so ExceptionScope.get
// falls back to the empty string), while the nested ROOTCAUSE keeps the
// throw's DETAIL/ERRORCODE/EXTENDEDINFO.
static cfvariant *buildRootCause(const webstrada::exception &ex)
{
    auto *root = new cfvariant(cfvariant::Struct);
    root->structSet("TYPE", cfvariant(ex.m_type));
    root->structSet("MESSAGE", cfvariant(ex.m_message));
    root->structSet("DETAIL", cfvariant(ex.m_detail));
    root->structSet("ERRORCODE", cfvariant(ex.m_errorCode));
    root->structSet("EXTENDEDINFO", cfvariant(ex.m_extendedInfo));
    root->structSet("CODE", cfvariant(ex.m_errorCode));
    root->structSet("EXTENDED_INFO", cfvariant(ex.m_extendedInfo));
    root->structSet("STACKTRACE", cfvariant(string()));
    cfvariant *tags = cfml::cf_stack_tagcontext(ex.m_stackTrace);
    root->structSet("TAGCONTEXT", *tags);
    delete tags;
    auto *suppressed = new cfvariant(cfvariant::Array);
    root->structSet("SUPPRESSED", *suppressed);
    delete suppressed;
    return root;
}

// The error/cferror struct (CfErrorWrapper + ExceptionScope) CF publishes for
// the exception handler page. The caller owns the returned variant.
static cfvariant *buildErrorStruct(const webstrada::exception &ex, const std::string &mailto,
                                   const string &generatedContent, const string &requestPath)
{
    auto *err = new cfvariant(cfvariant::Struct);
    err->structSet("TYPE", cfvariant("coldfusion.runtime.CfErrorWrapper"));
    err->structSet("MESSAGE", cfvariant(ex.m_message));
    err->structSet("GENERATEDCONTENT", cfvariant(generatedContent));
    err->structSet("MAILTO", cfvariant(mailto.c_str()));
    err->structSet("TEMPLATE", cfvariant(requestPath));
    // CF's Diagnostics appends "The error occurred in <template>: line N." to
    // the message/detail once the engine tracks the source location.
    webstrada::string diagnostics = ex.m_message + string(" ") + ex.m_detail;
    if (!ex.m_stackTrace.empty()) {
        const auto &loc = ex.m_stackTrace.back();
        diagnostics += " <br>The error occurred in ";
        diagnostics += webstrada::string(loc.path.c_str());
        diagnostics += ": line ";
        diagnostics += webstrada::string::number(loc.line);
        diagnostics += ".";
    }
    err->structSet("DIAGNOSTICS", cfvariant(diagnostics));
    err->structSet("BROWSER", cfvariant(string()));
    err->structSet("REMOTEADDRESS", cfvariant(string()));
    err->structSet("HTTPREFERER", cfvariant(string()));
    err->structSet("QUERYSTRING", cfvariant(string()));
    err->structSet("STACKTRACE", cfvariant(string()));
    cfvariant *tags = cfml::cf_stack_tagcontext(ex.m_stackTrace);
    err->structSet("TAGCONTEXT", *tags);
    delete tags;
    err->structSet("SUPPRESSED", cfvariant(false));
    // ExceptionScope.get pseudo-keys that resolve to "" (non-null) for the
    // wrapper: structKeyExists(error,'detail') is TRUE on CF and
    // #error.detail#/#error.errorcode#/#error.extendedinfo#/#error.exceptions#
    // read as empty. (The type-dependent pseudo-keys — nativeerrorcode,
    // sqlstate, errnumber, missingfilename, lockname, lockoperation — are NOT
    // present here, so they raise the undefined-variable error like CF.)
    err->structSet("DETAIL", cfvariant(string()));
    err->structSet("ERRORCODE", cfvariant(string()));
    err->structSet("EXTENDEDINFO", cfvariant(string()));
    err->structSet("EXCEPTIONS", cfvariant(string()));
    cfvariant *root = buildRootCause(ex);
    err->structSet("ROOTCAUSE", *root);
    delete root;

    // DATETIME: java.util.Date.toString() "EEE MMM dd HH:mm:ss zzz yyyy".
    std::time_t t = std::time(nullptr);
    struct tm tm_local;
    localtime_r(&t, &tm_local);
    char buf[64];
    if (strftime(buf, sizeof(buf), "%a %b %d %H:%M:%S %Z %Y", &tm_local) == 0) {
        buf[0] = '\0';
    }
    err->structSet("DATETIME", cfvariant(buf));
    return err;
}

// Runs one exception-handler template (CF's executeHandler): publishes the
// error/cferror structs into the shared variables scope, clears the output
// buffer, includes the template in the current request sharing all scopes.
// A <cfexit> inside the handler template ends only that template (include
// boundary); a non-abort exception propagates to the dispatch (which falls
// through to the request handler with it).
static void runHandlerPage(const CferrorHandler &h, const webstrada::exception &ex,
                           string *out, void *cgi, void *server, void *cookie,
                           void *application, void *session, void *url, void *form,
                           void *variables, const string &requestPath)
{
    cfml::IncludeRuntime *rt = cfml::include_context();
    if (!rt || !rt->loader) {
        throw webstrada::exception("cferror", "cferror is not available in this context.");
    }

    const string generatedContent = *out;
    cfvariant *errStruct = buildErrorStruct(ex, h.mailto, generatedContent, requestPath);
    if (variables && static_cast<cfvariant*>(variables)->m_type == cfvariant::Struct) {
        cfvariant *vars = static_cast<cfvariant*>(variables);
        vars->structSet("error", *errStruct);
        vars->structSet("cferror", *errStruct);
    }
    delete errStruct;

    out->clear();  // CF's NeoJspWriter.clear()

    cfml::include_template_fn target = rt->loader(h.templatePath.c_str(), rt->loaderOpaque);
    if (!target) {
        throw webstrada::exception("MissingInclude",
            ("Could not find the included template " + h.templatePath + ".").c_str(),
            "");
    }

    std::string savedPath = rt->currentPath;
    rt->currentPath = h.templatePath;
    try {
        target(out, cgi, server, cookie, application, session, url, form, variables);
    } catch (const webstrada::exit_exception &) {
        // <cfexit> in the handler template exits only that page.
    } catch (...) {
        rt->currentPath = savedPath;
        throw;
    }
    rt->currentPath = savedPath;
}

// Replaces a placeholder in `content` like CF's StringFunc.ReplaceNoCase with
// scope "ALL" (case-insensitive, every occurrence).
static void replacePlaceholder(string &content, const char *search, const string &replacement)
{
    cfvariant strVal(content);
    cfvariant sub1(search);
    cfvariant sub2(replacement);
    cfvariant scope("ALL");
    cfvariant *res = cfml::cf_replacenocase(&strVal, &sub1, &sub2, &scope);
    content = res->toString();
    delete res;
}

// CF's runRequestHandler + runSimpleHandler: reads the handler template as
// plain text (BOM stripped), clears the output buffer, replaces the 12
// placeholders, writes the result. Returns false when the template file is
// missing (falls through to the built-in handler).
static int runRequestHandler(const CferrorHandler &h, const webstrada::exception &ex,
                             string *out, const string &requestPath)
{
    if (h.templatePath.empty() || !fileExists(h.templatePath.c_str())) {
        return 0;
    }
    std::ifstream in(h.templatePath, std::ios::binary);
    if (!in) {
        return 0;
    }
    std::ostringstream ss;
    ss << in.rdbuf();
    std::string raw = ss.str();
    if (raw.size() >= 3 && (unsigned char)raw[0] == 0xEF &&
        (unsigned char)raw[1] == 0xBB && (unsigned char)raw[2] == 0xBF) {
        raw = raw.substr(3);  // strip UTF-8 BOM (CF's BOMReader)
    }

    const string generatedContent = *out;
    out->clear();

    string content(raw.c_str(), raw.size());
    replacePlaceholder(content, "#ERROR.GENERATEDCONTENT#", generatedContent);
    replacePlaceholder(content, "#ERROR.DIAGNOSTICS#",
                       ex.m_message + string(" ") + ex.m_detail);
    replacePlaceholder(content, "#ERROR.MAILTO#", string(h.mailto.c_str()));
    {
        std::time_t t = std::time(nullptr);
        struct tm tm_local;
        localtime_r(&t, &tm_local);
        char buf[64];
        if (strftime(buf, sizeof(buf), "%a %b %d %H:%M:%S %Z %Y", &tm_local) == 0) {
            buf[0] = '\0';
        }
        replacePlaceholder(content, "#ERROR.DATETIME#", string(buf));
    }
    replacePlaceholder(content, "#ERROR.BROWSER#", string());
    replacePlaceholder(content, "#ERROR.REMOTEADDRESS#", string());
    replacePlaceholder(content, "#ERROR.HTTPREFERER#", string());
    replacePlaceholder(content, "#ERROR.QUERYSTRING#", string());
    replacePlaceholder(content, "#ERROR.TEMPLATE#", requestPath);
    replacePlaceholder(content, "#ERROR.ROOTCAUSE.TYPE#", ex.m_type);
    replacePlaceholder(content, "#ERROR.ROOTCAUSE.MESSAGE#", ex.m_message);
    replacePlaceholder(content, "#ERROR.ROOTCAUSE.DETAIL#", ex.m_detail);

    out->append(content);
    return 1;
}

} // namespace

namespace cfml {

void cferror_reset()
{
    CferrorRuntime *reg = cferrorRuntime();
    reg->exceptionHandlers.clear();
    reg->requestHandler = CferrorHandler();
    reg->validationHandler = CferrorHandler();
    reg->invoked = false;
}

void cf_cferror_register(string *out, void *cgi, void *server, void *cookie, void *application,
                         void *session, void *url, void *form, void *variables,
                         const cfvariant *type, const cfvariant *templateAttr,
                         const cfvariant *mailto, const cfvariant *exception)
{
    cfml::IncludeRuntime *rt = cfml::include_context();
    if (!rt) {
        throw webstrada::exception("cferror", "cferror is not available in this context.");
    }

    string typeStr = type ? const_cast<cfvariant*>(type)->toString() : string();
    string tplStr = templateAttr ? const_cast<cfvariant*>(templateAttr)->toString() : string();
    string mailtoStr = mailto ? const_cast<cfvariant*>(mailto)->toString() : string();
    string excStr = exception ? const_cast<cfvariant*>(exception)->toString() : string("any");

    std::string tpl = tplStr.constData() ? std::string(tplStr.constData(), tplStr.length())
                                         : std::string();

    // 1. Template resolution FIRST (ErrorTag.doStartTag resolves the template
    //    before the type dispatch, so a bad template wins over a bad type).
    std::string resolved = resolveErrorTemplate(rt, tpl);

    const std::string typeLow = lowercase(
        typeStr.constData() ? std::string(typeStr.constData(), typeStr.length()) : std::string());
    const std::string excLow = lowercase(
        excStr.constData() ? std::string(excStr.constData(), excStr.length()) : std::string());

    CferrorRuntime *reg = cferrorRuntime();

    if (typeLow == "validation") {
        reg->validationHandler = CferrorHandler();
        reg->validationHandler.templatePath = resolved;
        reg->validationHandler.mailto = mailtoStr.constData() ? std::string(mailtoStr.constData(), mailtoStr.length()) : std::string();
        return;
    }
    if (typeLow == "request") {
        reg->requestHandler = CferrorHandler();
        reg->requestHandler.templatePath = resolved;
        reg->requestHandler.mailto = mailtoStr.constData() ? std::string(mailtoStr.constData(), mailtoStr.length()) : std::string();
        return;
    }
    if (typeLow == "exception") {
        CferrorHandler h;
        h.templatePath = resolved;
        h.mailto = mailtoStr.constData() ? std::string(mailtoStr.constData(), mailtoStr.length()) : std::string();
        const char *cls = exceptionClassForAttr(excLow);
        if (cls) {
            // Built-in names (and "any") register UNNAMED handlers.
            h.exceptionClass = cls;
            h.exceptionName = "";
        } else {
            // Custom exception name: 4-arg registration; the name is only
            // stored when non-empty (CF's isEmpty() check), so an empty
            // exception attribute registers an unnamed CustomException handler
            // that can never match (custom exceptions skip unnamed handlers).
            h.exceptionClass = "CustomException";
            h.exceptionName = excStr.constData() ? std::string(excStr.constData(), excStr.length()) : std::string();
        }
        addExceptionHandler(reg, h);
        return;
    }

    // ErrorTag.doStartTag's InvalidTagAttributeException (type Application):
    // message is the generic CFERROR validation line, detail carries the
    // invalid value (verified live: catchable, type "Application"). CF renders
    // an empty value as '' in the detail.
    std::string typeStr2 = typeStr.constData() ? std::string(typeStr.constData(), typeStr.length()) : std::string();
    if (typeStr2.empty()) {
        typeStr2 = "''";
    }
    throw webstrada::exception("Application", "Attribute validation error for tag CFERROR.",
        ("The value of the attribute type, which is currently " + typeStr2 + ", is invalid.").c_str());
}

int cf_cferror_handle(const webstrada::exception *ex, string *out, void *cgi, void *server,
                      void *cookie, void *application, void *session, void *url, void *form,
                      void *variables, const char *requestPath)
{
    if (!ex) {
        return 0;
    }
    CferrorRuntime *reg = cferrorRuntime();

    // CF's once-per-request "biscuit" marker: only the first exception per
    // request is dispatched (the cferror handlers must not run for an error
    // raised by the error page itself).
    if (reg->invoked) {
        return 0;
    }
    reg->invoked = true;

    // TemplateNotFoundException (a java.io.FileNotFoundException in CF, not a
    // MissingIncludeException) never reaches the <cferror> handlers; the
    // built-in 404 page handles it.
    if (ex->m_missingTemplate) {
        return 0;
    }

    const string requestPathStr = requestPath ? string(requestPath) : string();

    // runExceptionHandler.
    const webstrada::exception *current = ex;
    webstrada::exception fallbackEx;   // a handler-template exception survives
    bool haveFallback = false;        // the dispatch (must outlive the catch)
    CferrorHandler *anyHandler = nullptr;
    CferrorHandler *handler = matchExceptionHandler(reg, *current, &anyHandler);
    if (!handler && anyHandler && current->m_isCustom) {
        handler = anyHandler;
    }
    if (handler) {
        try {
            runHandlerPage(*handler, *current, out, cgi, server, cookie, application,
                           session, url, form, variables, requestPathStr);
            return 1;
        } catch (const webstrada::abort_exception &) {
            // <cfabort> in the handler template: handled (CF returns true).
            return 1;
        } catch (const webstrada::exit_exception &) {
            return 1;
        } catch (const webstrada::exception &newEx) {
            // The handler template threw: continue with the NEW exception
            // (exception handlers are not re-run, matching CF).
            fallbackEx = newEx;
            haveFallback = true;
        }
    }
    if (haveFallback) {
        current = &fallbackEx;
    }

    // Site-wide (admin-configured) handler: none in this engine.

    // runRequestHandler.
    if (!reg->requestHandler.templatePath.empty()) {
        try {
            if (runRequestHandler(reg->requestHandler, *current, out, requestPathStr)) {
                cfml::response().statusCode = 500;
                return 1;
            }
        } catch (...) {
            // The request handler itself failed: swallow (CF catches Throwable).
            return 1;
        }
    }

    return 0;
}

} // namespace cfml
