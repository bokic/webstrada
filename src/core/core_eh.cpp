#include "core_internal.h"
#include "../cftags/common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <webstrada/parser.h>
#include <webstrada/worker.h>
#include <webstrada/cfimage.h>
#include <webstrada/cfvariant.h>
#include <webstrada/string.h>
#include <webstrada/scope_store.h>
#include <webstrada/config.h>
#include <webstrada/locale.h>
#include <webstrada/cfimage.h>

#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>
#include <sqlite3.h>
#include <curl/curl.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/hmac.h>
#include <openssl/provider.h>

#include <thread>
#include <chrono>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <ctime>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <fstream>
#include <filesystem>
#include <unistd.h>
#include <fcntl.h>

using namespace webstrada;
using namespace cfml;
// ---- JIT exception-handling helpers ----
// ---- JIT try/catch exception-handling helpers -------------------------------
//
// The JIT landing pad receives the Itanium exception object pointer (the
// `landingpad` element 0) and passes it to these helpers. All reads go through
// __cxa_get_exception_ptr/__cxa_begin_catch so they see the adjusted object
// exactly as a C++ `catch` would.

namespace {

const webstrada::exception *caughtAsException(const void *exn)
{
    if (!exn) return nullptr;
    return static_cast<const webstrada::exception*>(exn);
}

// ---- CF exception-class model ----
//
// Adobe ColdFusion matches a `catch (type)` clause by Java exception class,
// not by exact type-name equality. Each catch type resolves to a target class
// (via NeoException's registry) and the clause whose target class is the
// *closest ancestor* of the thrown exception's class wins (findThrowableTarget);
// "any" maps to java.lang.Exception so it always matches (with the largest
// distance). Exceptions raised by user `throw`/`<cfthrow>` are CustomException
// and are matched by exact type name (plus dotted-prefix) over `any`
// (findCustomTarget). The hierarchy below mirrors the CF 2025 classes.
//
// Target classes per catch type (NeoException.vExceptions registry):
//   database -> DatabaseException     template -> TemplateException
//   security -> AccessControlException object  -> ObjectException
//   missinginclude -> MissingIncludeException   expression -> RuntimeException
//   lock -> LockException              application -> ApplicationException
//   searchengine -> SearchEngineException        any -> java.lang.Exception
//
// Class parent links (each child's superclass):
//   Exception (root) <- RuntimeException <- NeoException <-
//     ExpressionException, ApplicationException <- TemplateException <-
//       MissingIncludeException, CustomException, DatabaseException,
//       LockException, AccessControlException, ObjectException, SearchEngineException

struct ClassNode {
    const char *name;
    const char *parent;   // nullptr for java.lang.Exception (root)
};

static const ClassNode kClassHierarchy[] = {
    {"java.lang.Throwable", nullptr},
    {"java.lang.Exception", "java.lang.Throwable"},
    {"java.lang.RuntimeException", "java.lang.Exception"},
    {"NeoException", "java.lang.RuntimeException"},
    {"ExpressionException", "NeoException"},
    {"ApplicationException", "NeoException"},
    {"TemplateException", "ApplicationException"},
    {"MissingIncludeException", "TemplateException"},
    {"CustomException", "ApplicationException"},
    {"DatabaseException", "NeoException"},
    {"LockException", "NeoException"},
    {"AccessControlException", "NeoException"},
    {"ObjectException", "NeoException"},
    {"SearchEngineException", "NeoException"},
};

// The target class a catch clause type resolves to (CF's getTargetClass).
// Returns nullptr when the type is not a registered built-in class (a custom
// catch name — only relevant to custom exceptions' findCustomTarget).
static const char *targetClassForCatch(const char *cfType)
{
    if (!cfType) return nullptr;
    webstrada::string t(cfType);
    t.toUpper();
    if (t.equals("DATABASE")) return "DatabaseException";
    if (t.equals("TEMPLATE")) return "TemplateException";
    if (t.equals("SECURITY")) return "AccessControlException";
    if (t.equals("OBJECT")) return "ObjectException";
    if (t.equals("MISSINGINCLUDE")) return "MissingIncludeException";
    if (t.equals("EXPRESSION")) return "java.lang.RuntimeException";
    if (t.equals("LOCK")) return "LockException";
    if (t.equals("APPLICATION")) return "ApplicationException";
    if (t.equals("SEARCHENGINE")) return "SearchEngineException";
    if (t.equals("ANY")) return "java.lang.Exception";
    return nullptr;
}

// CF's findCustomTarget: an exact type-name match (case-insensitive) wins
// immediately over `any`; otherwise a dotted-prefix partial match is the best;
// otherwise `any` (the largest distance) — so a custom throw only ever matches
// its exact name or `any`, never `expression`/`application`/etc.
static int findCustomTarget(const webstrada::string &exceptionType, const char **types, int count)
{
    int result = -1;
    int bestDiff = INT_MAX;
    for (int i = 0; i < count; i++) {
        const char *strTarget = types[i] ? types[i] : "";
        webstrada::string target(strTarget);
        if (exceptionType.compareCaseInsensitive(target) == 0) {
            return i;
        }
        // Dotted-prefix partial match: e.g. thrown type "my.custom.type" with
        // a catch of "my.custom" (diff 1) or "my" (diff 2).
        int thisDiff = INT_MAX;
        webstrada::string cfmlToks = exceptionType;
        webstrada::string targetToks = target;
        std::vector<webstrada::string> cfmlParts, targetParts;
        int pos = 0;
        while (pos < cfmlToks.length()) {
            int dot = cfmlToks.indexOf('.', pos);
            if (dot < 0) { cfmlParts.push_back(cfmlToks.mid(pos, cfmlToks.length() - pos)); break; }
            cfmlParts.push_back(cfmlToks.mid(pos, dot - pos));
            pos = dot + 1;
        }
        pos = 0;
        while (pos < targetToks.length()) {
            int dot = targetToks.indexOf('.', pos);
            if (dot < 0) { targetParts.push_back(targetToks.mid(pos, targetToks.length() - pos)); break; }
            targetParts.push_back(targetToks.mid(pos, dot - pos));
            pos = dot + 1;
        }
        int partialMatchDiff = static_cast<int>(cfmlParts.size()) - static_cast<int>(targetParts.size());
        bool b1 = partialMatchDiff > 0;
        bool b2 = partialMatchDiff <= 0;
        if (partialMatchDiff > 0) {
            size_t j = 0;
            while (j < targetParts.size() && j < cfmlParts.size()) {
                if (cfmlParts[j].compareCaseInsensitive(targetParts[j]) != 0) break;
                j++;
            }
            if (j == targetParts.size()) {
                thisDiff = partialMatchDiff;
            }
        }
        if (thisDiff == INT_MAX && target.compareCaseInsensitive("ANY") == 0) {
            thisDiff = INT_MAX - 1;
        }
        if (thisDiff < bestDiff) {
            result = i;
            bestDiff = thisDiff;
        }
    }
    return result;
}

// CF's findThrowableTarget for built-in (non-custom) exceptions: pick the clause
// whose target class is the closest ancestor of the thrown exception's class.
static int findThrowableTarget(const webstrada::exception *e, const char **types, int count)
{
    const char *cls = cfml::cf_eh_thrown_class(e->m_type);
    int result = -1;
    int bestDiff = INT_MAX;
    for (int i = 0; i < count; i++) {
        const char *targetClass = targetClassForCatch(types[i]);
        if (!targetClass) continue;
        int diff = cfml::cf_eh_class_distance(cls, targetClass);
        if (diff < bestDiff) {
            bestDiff = diff;
            result = i;
        }
    }
    return result;
}

}

// Number of superclass transitions from `cls` up to `target`, or INT_MAX when
// `target` is not an ancestor (CF's diffClassTypes).
int cfml::cf_eh_class_distance(const char *cls, const char *target)
{
    if (!cls || !target) return INT_MAX;
    int transitions = 0;
    const char *cur = cls;
    while (cur) {
        if (strcmp(cur, target) == 0) return transitions;
        if (strcmp(cur, "java.lang.Throwable") == 0) break;   // reached root
        const char *next = nullptr;
        for (const auto &node : kClassHierarchy) {
            if (strcmp(node.name, cur) == 0) { next = node.parent; break; }
        }
        if (!next) break;
        cur = next;
        transitions++;
    }
    return INT_MAX;
}

// The exception class a built-in (non-custom) thrown type maps to. Returns the
// canonical class name; unknown engine-internal types map to NeoException so
// they remain catchable by `expression`/`any` like CF's RuntimeException errors.
const char *cfml::cf_eh_thrown_class(const webstrada::string &type)
{
    webstrada::string t = type;
    t.toUpper();
    if (t.equals("EXPRESSION")) return "ExpressionException";
    if (t.equals("APPLICATION")) return "ApplicationException";
    if (t.equals("DATABASE")) return "DatabaseException";
    if (t.equals("TEMPLATE")) return "TemplateException";
    if (t.equals("LOCK")) return "LockException";
    if (t.equals("SECURITY")) return "AccessControlException";
    if (t.equals("OBJECT")) return "ObjectException";
    if (t.equals("MISSINGINCLUDE")) return "MissingIncludeException";
    if (t.equals("SEARCHENGINE")) return "SearchEngineException";
    return "NeoException";
}


bool cfml::cf_eh_matches(const void *exn, const char *cfType)
{
    if (!exn || !cfType) return false;
    const webstrada::exception *e = caughtAsException(abi::__cxa_get_exception_ptr(const_cast<void*>(exn)));
    if (!e || !e->catchable()) return false;
    const char *t[1] = { cfType };
    return cfml::cf_eh_best_match(exn, t, 1) == 0;
}

int cfml::cf_eh_best_match(const void *exn, const char **types, int count)
{
    if (!exn || !types || count <= 0) return -1;
    const webstrada::exception *e = caughtAsException(abi::__cxa_get_exception_ptr(const_cast<void*>(exn)));
    if (!e || !e->catchable()) return -1;
    if (e->m_isCustom) {
        return findCustomTarget(e->m_type, types, count);
    }
    return findThrowableTarget(e, types, count);
}

cfvariant *cfml::cf_eh_capture(void *exn)
{
    if (!exn) {
        throw webstrada::exception("Expression", "An error occurred while processing the request.", "No exception object available in catch block.");
    }
    // Marks the exception caught and returns the adjusted object pointer. The
    // matching __cxa_end_catch() below destroys the C++ exception so a caught
    // CFML exception never leaks.
    const webstrada::exception *e = caughtAsException(abi::__cxa_begin_catch(exn));

    auto *ret = new cfvariant(cfvariant::Struct);
    ret->structSet("TYPE", cfvariant(e ? e->m_type : webstrada::string("Expression")));
    ret->structSet("MESSAGE", cfvariant(e ? e->m_message : webstrada::string()));
    ret->structSet("DETAIL", cfvariant(e ? e->m_detail : webstrada::string()));
    ret->structSet("ERRORCODE", cfvariant(e ? e->m_errorCode : webstrada::string()));
    ret->structSet("EXTENDEDINFO", cfvariant(e ? e->m_extendedInfo : webstrada::string()));
    ret->m_isCustomException = e && e->m_isCustom;
    ret->m_isAbort = e && !e->catchable();
    // The call-stack snapshot was captured into the exception's m_stackTrace by
    // the first landing pad the unwinding exception reached; build the CFML
    // TAGCONTEXT array from it (empty when nothing was captured).
    static const std::vector<webstrada::StackLevel> kNoStack;
    cfvariant *tags = cfml::cf_stack_tagcontext(e ? e->m_stackTrace : kNoStack);
    ret->structSet("TAGCONTEXT", *tags);
    delete tags;
    cf_register_temp(ret);

    abi::__cxa_end_catch();
    return ret;
}

void cfml::cf_eh_throw(cfvariant *ex)
{
    // Rethrow path (cfrethrow / finally re-raise / transaction rollback / the
    // unmatched path of a <cftry>): the exception was already normalized when it
    // was first thrown/captured, so no reserved-type validation happens here —
    // just preserve the custom flag. An uncatchable abort is re-raised as an
    // abort_exception so <cfabort>/<cflocation> inside a <cftry> still abort
    // (was BUGS.md "abort inside cftry is re-raised as a catchable Request");
    // an uncatchable <cfexit> is re-raised as an exit_exception so it keeps
    // exiting only the current page (the include boundary swallows it).
    if (ex && ex->m_type == cfvariant::Struct && ex->m_isAbort) {
        webstrada::string caughtType = ex->has("TYPE") ? (*ex)["TYPE"].toString() : webstrada::string();
        caughtType.toLower();
        if (caughtType.equals("template")) {
            throw webstrada::exit_exception();
        }
        throw webstrada::abort_exception();
    }
    webstrada::string type("Application");
    webstrada::string message, detail, errorCode, extendedInfo;
    bool isCustom = false;
    if (ex && ex->m_type == cfvariant::Struct) {
        if (ex->has("TYPE")) type = (*ex)["TYPE"].toString();
        if (ex->has("MESSAGE")) message = (*ex)["MESSAGE"].toString();
        if (ex->has("DETAIL")) detail = (*ex)["DETAIL"].toString();
        if (ex->has("ERRORCODE")) errorCode = (*ex)["ERRORCODE"].toString();
        if (ex->has("EXTENDEDINFO")) extendedInfo = (*ex)["EXTENDEDINFO"].toString();
        isCustom = ex->m_isCustomException;
    }
    webstrada::exception e(type, message, detail);
    e.m_errorCode = errorCode;
    e.m_extendedInfo = extendedInfo;
    e.m_isCustom = isCustom;
    // Preserve the original stack trace across a rethrow (<cfrethrow> / finally
    // re-raise / the unmatched path of a <cftry>): CF rethrows the same Java
    // exception, so the tagContext keeps the throw-site frames instead of being
    // re-captured at the rethrow line.
    if (ex && ex->m_type == cfvariant::Struct && ex->has("TAGCONTEXT")) {
        e.m_stackTrace = cfml::cf_stack_tagcontext_to_levels(&(*ex)["TAGCONTEXT"]);
    }
    throw e;
}

void cfml::cf_eh_throw_new(cfvariant *ex, int isFunction)
{
    // Fresh user throw (<cfthrow> / script throw). CF 2025 rejects throws of the
    // reserved built-in type names (ThrowTag.validate): the throw is replaced by
    // an InvalidTagAttributeException (an ApplicationException) reporting type
    // "Application" with a validation message, so `throw(type="database")` does
    // NOT match catch(database). Everything else is a CustomException matched by
    // exact name (m_isCustom = true).
    webstrada::string type("Application");
    webstrada::string message, detail, errorCode, extendedInfo;
    bool complexMessage = false;
    if (ex && ex->m_type == cfvariant::Struct) {
        if (ex->has("TYPE")) type = (*ex)["TYPE"].toString();
        if (ex->has("MESSAGE")) message = (*ex)["MESSAGE"].toString();
        if (ex->has("DETAIL")) detail = (*ex)["DETAIL"].toString();
        if (ex->has("ERRORCODE")) errorCode = (*ex)["ERRORCODE"].toString();
        if (ex->has("EXTENDEDINFO")) extendedInfo = (*ex)["EXTENDEDINFO"].toString();
        // A complex message value (throw someStruct / throw arr / throw query)
        // is rejected by CF with an Expression error (verified on CF 2025:
        // throw s where s = {a:1} -> [Expression]Complex object types cannot
        // be converted to simple values. with CF's standard detail).
        if (ex->has("MESSAGE")) {
            const cfvariant &mval = (*ex)["MESSAGE"];
            if (mval.m_type == cfvariant::Struct || mval.m_type == cfvariant::Array ||
                mval.m_type == cfvariant::Query || mval.m_type == cfvariant::Component ||
                mval.m_type == cfvariant::Xml || mval.m_type == cfvariant::Binary ||
                mval.m_type == cfvariant::Image) {
                complexMessage = true;
            }
        }
    }
    if (complexMessage) {
        throw webstrada::exception("Expression",
            "Complex object types cannot be converted to simple values.",
            "The expression has requested a variable or an intermediate expression result as a simple value. However, the result cannot be converted to a simple value. Simple values are strings, numbers, boolean values, and date/time values. Queries, arrays, and COM objects are examples of complex values. <p>The most likely cause of the error is that you tried to use a complex value as a simple one. For example, you tried to use a query variable in a cfif tag.");
    }
    bool isCustom = true;
    webstrada::string t = type;
    t.toUpper();
    static const char *kReserved[] = {
        "DATABASE", "TEMPLATE", "SECURITY", "OBJECT",
        "MISSINGINCLUDE", "EXPRESSION", "LOCK"
    };
    for (const char *r : kReserved) {
        if (t.equals(r)) {
            type = "Application";
            message = isFunction
                ? "Attribute validation error for function throw."
                : "Attribute validation error for tag CFTHROW.";
            isCustom = false;
            break;
        }
    }
    webstrada::exception e(type, message, detail);
    e.m_errorCode = errorCode;
    e.m_extendedInfo = extendedInfo;
    e.m_isCustom = isCustom;
    throw e;
}

namespace {
    thread_local std::vector<cfvariant*> g_temp_variants;
}

void cfml::cf_register_temp(cfvariant *v) {
    if (!v) return;
    char stackDummy;
    uintptr_t stackAddr = reinterpret_cast<uintptr_t>(&stackDummy);
    uintptr_t valAddr = reinterpret_cast<uintptr_t>(v);
    // Ignore stack pointers (pointers within 16MB of current thread stack)
    if (valAddr >= stackAddr - 16777216 && valAddr <= stackAddr + 16777216) {
        return;
    }
    // A fresh result can reach cf_register_temp through several ownership paths
    // (the JIT's emitCall whitelist, the builtin-dispatch wrapper, the
    // interpreter, or the producing function itself); register each object
    // exactly once so the cleanup never deletes the same object twice.
    if (v->m_tempRegistered) return;
    v->m_tempRegistered = true;
    g_temp_variants.push_back(v);
}

namespace {

// True when `c` is one of the delimiter characters in `delims`.
bool isDelim(char c, const webstrada::string &delims)
{
    return delims.indexOf(c) >= 0;
}

// Number of non-empty items when `s` is treated as a list whose delimiters are
// any character in `delims` (CF's list semantics skip empty elements).
long long listItemCount(const webstrada::string &s, const webstrada::string &delims)
{
    long long count = 0;
    size_t itemStart = 0;
    for (size_t i = 0; i <= s.length(); i++) {
        if (i == s.length() || isDelim(s.at(i), delims)) {
            if (i > itemStart) count++;
            itemStart = i + 1;
        }
    }
    return count;
}

webstrada::string listItemAt(const webstrada::string &s, const webstrada::string &delims, long long index)
{
    long long count = 0;
    size_t itemStart = 0;
    for (size_t i = 0; i <= s.length(); i++) {
        if (i == s.length() || isDelim(s.at(i), delims)) {
            if (i > itemStart) {
                count++;
                if (count == index) return s.mid(itemStart, i - itemStart);
            }
            itemStart = i + 1;
        }
    }
    return webstrada::string();
}

}

long long cfml::cfforInLength(const webstrada::cfvariant *coll, const webstrada::cfvariant *delims)
{
    if (!coll) return 0;
    webstrada::string dels = (delims && delims->m_type == webstrada::cfvariant::String && delims->m_str)
        ? *delims->m_str
        : webstrada::string(",");
    switch (coll->m_type) {
        case webstrada::cfvariant::Array:
            if (isQueryColumnRef(coll)) throwNotArrayError(coll);
            return coll->m_array ? static_cast<long long>(coll->m_array->size()) : 0;
        case webstrada::cfvariant::Struct:
            if (coll->m_isArguments) {
                return static_cast<long long>(argumentsVisibleKeys(coll).size());
            }
            if (coll->m_structInsertOrder) return static_cast<long long>(coll->m_structInsertOrder->size());
            return coll->m_struct ? static_cast<long long>(coll->m_struct->size()) : 0;
        case webstrada::cfvariant::String:
            if (!coll->m_str) return 0;
            return listItemCount(*coll->m_str, dels);
        default:
            return 0;
    }
}

cfvariant *cfml::cfforInItem(const webstrada::cfvariant *coll, long long index, const webstrada::cfvariant *delims)
{
    if (!coll) throw webstrada::exception("Cannot iterate over a null value");
    webstrada::string dels = (delims && delims->m_type == webstrada::cfvariant::String && delims->m_str)
        ? *delims->m_str
        : webstrada::string(",");
    switch (coll->m_type) {
        case webstrada::cfvariant::Array: {
            if (isQueryColumnRef(coll)) throwNotArrayError(coll);
            if (!coll->m_array) throw webstrada::exception("Cannot iterate over a null array");
            if (index < 1 || index > static_cast<long long>(coll->m_array->size()))
                throw webstrada::exception("Array index out of bounds");
            return &coll->m_array->at(static_cast<size_t>(index - 1));
        }
        case webstrada::cfvariant::Struct: {
            std::vector<webstrada::string> keys;
            if (coll->m_isArguments) {
                keys = argumentsVisibleKeys(coll);
            } else if (coll->m_structInsertOrder) {
                keys = *coll->m_structInsertOrder;
            } else if (coll->m_struct) {
                // No insertion order recorded (e.g. a struct built by legacy
                // paths); fall back to the map's sorted iteration order.
                for (const auto &kv : *coll->m_struct) keys.push_back(kv.first);
            }
            if (keys.empty() || index < 1 || index > static_cast<long long>(keys.size()))
                throw webstrada::exception("Struct index out of bounds");
            auto *ret = new webstrada::cfvariant(keys.at(static_cast<size_t>(index - 1)));
            cf_register_temp(ret);
            return ret;
        }
        case webstrada::cfvariant::String: {
            if (!coll->m_str) throw webstrada::exception("Cannot iterate over a null string");
            webstrada::string item = listItemAt(*coll->m_str, dels, index);
            auto *ret = new webstrada::cfvariant(item);
            cf_register_temp(ret);
            return ret;
        }
        default:
            throw webstrada::exception("Cannot iterate over this type with a for-in loop");
    }
}

cfvariant cfml::cfevaluate(string &out, const string &expr, cfvariant *variables)
{
    return evaluateExpr(out, expr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, variables);
}

size_t cfml::cfvariant_cleanup_save() {
    return g_temp_variants.size();
}

void cfml::cfvariant_cleanup_restore(size_t savepoint) {
    while (g_temp_variants.size() > savepoint) {
        delete g_temp_variants.back();
        g_temp_variants.pop_back();
    }
}

void cfml::cfvariant_cleanup_restore_except(size_t savepoint, cfvariant *retVal) {
    bool preserved = false;
    cfvariant *capturedScope = nullptr;
    if (retVal && retVal->m_type == cfvariant::Function && retVal->m_udf) {
        capturedScope = retVal->m_udf->capturedScope;
    }
    bool preservedCaptured = false;
    std::vector<cfvariant*> toDelete;
    while (g_temp_variants.size() > savepoint) {
        cfvariant *v = g_temp_variants.back();
        g_temp_variants.pop_back();
        if (v == retVal && !preserved) {
            preserved = true;
        } else if (v == capturedScope && !preservedCaptured) {
            preservedCaptured = true;
        } else if (v) {
            toDelete.push_back(v);
        }
    }
    if (preservedCaptured && capturedScope) {
        capturedScope->m_tempRegistered = false;
        cf_register_temp(capturedScope);
    }
    if (preserved && retVal) {
        retVal->m_tempRegistered = false;
        cf_register_temp(retVal);
    }
    for (cfvariant *v : toDelete) {
        delete v;
    }
}
// JSON serialization helpers

// Renders a computed double exactly the way ColdFusion's SerializeJSON does
// for computed (non-literal) doubles: Java's Double.toString — the shortest
// decimal that round-trips, scientific notation when the decimal exponent is
// outside [-3, 6] ("1.0E7", "1.0E-4", "9.007199254740992E15"). Literal
// floats are unaffected — they carry their original text in m_literalText.

cfvariant *cfml::cf_asc(const cfvariant *arg) {
    if (!arg) throw webstrada::exception("Asc requires exactly 1 argument");
    string s = const_cast<cfvariant*>(arg)->toString();
    const char *data = s.constData();
    if (!data || data[0] == '\0') {
        throw webstrada::exception("Asc: string cannot be empty");
    }
    auto *ret = new cfvariant(static_cast<int>(static_cast<unsigned char>(data[0])));
    return ret;
}

cfvariant *cfml::cf_chr(const cfvariant *arg) {
    if (!arg) throw webstrada::exception("Chr requires exactly 1 argument");
    int cp = getIntValue(*arg);
    if (cp < 0 || cp > 65535) {
        throw webstrada::exception("Chr: Number must be in the range 0 - 65535");
    }

    char bytes[4] = {0};
    size_t len = 0;
    if (cp <= 0x7F) {
        bytes[0] = static_cast<char>(cp);
        len = 1;
    } else if (cp <= 0x7FF) {
        bytes[0] = static_cast<char>(0xC0 | ((cp >> 6) & 0x1F));
        bytes[1] = static_cast<char>(0x80 | (cp & 0x3F));
        len = 2;
    } else {
        bytes[0] = static_cast<char>(0xE0 | ((cp >> 12) & 0x0F));
        bytes[1] = static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        bytes[2] = static_cast<char>(0x80 | (cp & 0x3F));
        len = 3;
    }

    auto *ret = new cfvariant(string(bytes, len));
    return ret;
}

cfvariant *cfml::cfvariant_create_null() {
    auto *v = new cfvariant(cfvariant::Null);
    return v;
}

cfvariant *cfml::cfvariant_create_int(int val) {
    auto *v = new cfvariant(val);
    return v;
}

cfvariant *cfml::cfvariant_create_long(long long val) {
    auto *v = new cfvariant(cfvariant::Long);
    v->m_long = val;
    return v;
}

cfvariant *cfml::cfvariant_create_float(double val) {
    auto *v = new cfvariant(cfvariant::Float);
    v->m_double = val;
    return v;
}

cfvariant *cfml::cfvariant_create_float_literal(const char *text, double val) {
    auto *v = new cfvariant(cfvariant::Float);
    v->m_double = val;
    if (text && *text) {
        v->m_literalText = new string(text);
    }
    return v;
}

cfvariant *cfml::cfvariant_create_bool(bool val) {
    auto *v = new cfvariant(cfvariant::Boolean);
    v->m_bool = val;
    return v;
}

cfvariant *cfml::cfvariant_create_bool_literal(bool val) {
    auto *v = new cfvariant(cfvariant::Boolean);
    v->m_bool = val;
    v->m_boolLiteral = true;
    return v;
}

cfvariant *cfml::cfvariant_create_string(const char *val) {
    auto *v = new cfvariant(val);
    return v;
}

cfvariant *cfml::cfvariant_create_array() {
    auto *v = new cfvariant(cfvariant::Array);
    return v;
}

cfvariant *cfml::cfvariant_create_struct() {
    auto *v = new cfvariant(cfvariant::Struct);
    // The JIT's emitCall whitelist explicitly excludes cfvariant_create_struct
    // (its result is used as a mutable workspace then copied into a scope, so
    // the object itself must still be freed by the request cleanup).
    cf_register_temp(v);
    return v;
}

// CF reports a missing member of an enabled scope as "Element X is undefined in
// SESSION." (member path uppercased, scope name uppercased) instead of the
// generic "Variable X is undefined." — see BUGS.md "APPLICATION/SESSION scope
// error message for missing elements". When `name` is a scope-qualified dotted
// name whose first segment is a known scope that is present (non-null, enabled
// struct), throws CF's message and returns true; otherwise returns false so the
// caller falls back to its generic undefined-variable message.
bool cfml::cf_throw_scope_member_error(const char *name,
    const cfvariant *cgi, const cfvariant *server,
    const cfvariant *cookie, const cfvariant *application,
    const cfvariant *session, const cfvariant *url,
    const cfvariant *form, const cfvariant *variables)
{
    if (!name || !*name) return false;
    std::string n(name);
    size_t dot = n.find('.');
    if (dot == std::string::npos) return false;
    std::string head = n.substr(0, dot);
    for (auto &c : head) c = static_cast<char>(toupper(static_cast<unsigned char>(c)));

    struct { const char *n; const cfvariant *p; } scopes[] = {
        {"VARIABLES", variables}, {"CGI", cgi}, {"URL", url},
        {"FORM", form}, {"COOKIE", cookie}, {"SERVER", server},
        {"APPLICATION", application}, {"SESSION", session},
    };
    const cfvariant *scope = nullptr;
    for (auto &s : scopes) {
        if (head == s.n) { scope = s.p; break; }
    }
    if (!scope || scope->m_type != cfvariant::Struct || scope->m_disabled) return false;

    // The member path is the dotted remainder after the scope, uppercased.
    std::string rest = n.substr(dot + 1);
    for (auto &c : rest) c = static_cast<char>(toupper(static_cast<unsigned char>(c)));
    std::string msg = "Element " + rest + " is undefined in " + head + ".";
    throw webstrada::exception(msg.c_str());
}

void cfml::cf_throw_array_oob(int idx, int dimension, const char *varName)
{
    // CF's ArrayIndexOutOfBounds message (was BUGS.md "Array index out-of-bounds
    // error message"). The quote characters are HTML entities (&quot;) literally,
    // matching CF's stored message. When the base is a simple named variable the
    // variable name is included; otherwise the array-object form is used.
    webstrada::string msg;
    if (varName && *varName) {
        webstrada::string name(varName);
        name.toUpper();
        msg.append("The element at position ");
        msg.append(webstrada::string::number(idx));
        msg.append(" of dimension ");
        msg.append(webstrada::string::number(dimension));
        msg.append(", of array variable &quot;");
        msg.append(name);
        msg.append(",&quot; cannot be found.");
    } else {
        msg.append("The element at position ");
        msg.append(webstrada::string::number(idx));
        msg.append(", of dimension ");
        msg.append(webstrada::string::number(dimension));
        msg.append(", of an array object used as part of an expression, cannot be found.");
    }
    throw webstrada::exception(msg);
}

const cfvariant *cfml::cfvariant_get_var(
    const cfvariant *cgi, const cfvariant *server,
    const cfvariant *cookie, const webstrada::cfvariant *application,
    const cfvariant *session, const cfvariant *url,
    const cfvariant *form, const cfvariant *variables,
    const char *name)
{
    auto *v = lookupVarWritable(name,
        const_cast<cfvariant*>(cgi),
        const_cast<cfvariant*>(server),
        const_cast<cfvariant*>(cookie),
        const_cast<cfvariant*>(application),
        const_cast<cfvariant*>(session),
        const_cast<cfvariant*>(url),
        const_cast<cfvariant*>(form),
        const_cast<cfvariant*>(variables));
    if (!v) {
        if (cf_throw_scope_member_error(name, cgi, server, cookie, application, session, url, form, variables)) {
            // unreachable: cf_throw_scope_member_error throws
        }
        // CF uppercases the name and does not quote it: `victim[1]` on an
        // undefined base throws "Variable VICTIM is undefined." (was BUGS.md
        // "chain-base lookups": previously "Variable 'victim' is undefined.").
        string root(name);
        root.toUpper();
        throw webstrada::exception(string("Variable ") + root + " is undefined.");
    }
    return v;
}

// Resolves a chain base for DOT member access and reads the named member. On an
// undefined base, CF reports the ELEMENT message ("Element KEY is undefined in
// BASE.") rather than the variable message used for bracket access — this is the
// `.key` chain-base divergence (was BUGS.md "chain-base lookups").
cfvariant *cfml::cfvariant_get_member(
    const cfvariant *cgi, const cfvariant *server,
    const cfvariant *cookie, const webstrada::cfvariant *application,
    const cfvariant *session, const cfvariant *url,
    const cfvariant *form, const cfvariant *variables,
    const char *name, const char *key)
{
    auto *v = lookupVarWritable(name,
        const_cast<cfvariant*>(cgi),
        const_cast<cfvariant*>(server),
        const_cast<cfvariant*>(cookie),
        const_cast<cfvariant*>(application),
        const_cast<cfvariant*>(session),
        const_cast<cfvariant*>(url),
        const_cast<cfvariant*>(form),
        const_cast<cfvariant*>(variables));
    if (!v) {
        if (cf_throw_scope_member_error(name, cgi, server, cookie, application, session, url, form, variables)) {
            // unreachable: cf_throw_scope_member_error throws
        }
        string upKey(key);
        upKey.toUpper();
        string upBase(name);
        upBase.toUpper();
        webstrada::string msg("Element ");
        msg.append(upKey);
        msg.append(" is undefined in ");
        msg.append(upBase);
        msg.append(".");
        throw webstrada::exception(msg);
    }
    // Index the resolved base with the key (cfvariant_index auto-creates a
    // missing struct key, mirroring CF's read-through on an existing struct).
    cfvariant *keyVal = new cfvariant(key);
    cf_register_temp(keyVal);
    return cfml::cfvariant_index(v, keyVal);
}

cfvariant *cfml::cfvariant_bare_identifier(
    const cfvariant *cgi, const cfvariant *server,
    const cfvariant *cookie, const cfvariant *application,
    const cfvariant *session, const cfvariant *url,
    const cfvariant *form, const cfvariant *variables,
    const char *name)
{
    auto *v = lookupVarWritable(name,
        const_cast<cfvariant*>(cgi),
        const_cast<cfvariant*>(server),
        const_cast<cfvariant*>(cookie),
        const_cast<cfvariant*>(application),
        const_cast<cfvariant*>(session),
        const_cast<cfvariant*>(url),
        const_cast<cfvariant*>(form),
        const_cast<cfvariant*>(variables));
    if (v) return v;

    string sname(name);
    if (isKnownFunctionName(sname)) {
        auto *ret = new cfvariant(cfvariant::Function);
        *ret->m_str = functionHandleText(sname);
        // cfvariant_bare_identifier is in the JIT emitCall exclusion list
        // (it may return a borrowed scope pointer), so register the fresh
        // method-handle result here.
        cf_register_temp(ret);
        return ret;
    }

    if (cf_throw_scope_member_error(name, cgi, server, cookie, application, session, url, form, variables)) {
        // unreachable: cf_throw_scope_member_error throws
    }

    // A dotted name whose lookup failed reports CF's message on the undefined
    // base (was BUGS.md "chain-base lookups"). A single-dot access
    // (undefinedStruct.key) -> "Element KEY is undefined in UNDEFINEDSTRUCT.";
    // a multi-dot access (undefinedStruct.a.b) and a bare name -> "Variable
    // UNDEFINEDSTRUCT is undefined." (the root base only).
    webstrada::string nm(name);
    int dot = nm.indexOf('.');
    if (dot > 0) {
        string member = nm.mid(dot + 1, nm.length() - dot - 1).trimmed();
        if (member.indexOf('.') < 0 && member.indexOf('[') < 0) {
            string base = nm.left(dot).trimmed();
            member.toUpper();
            base.toUpper();
            webstrada::string msg("Element ");
            msg.append(member);
            msg.append(" is undefined in ");
            msg.append(base);
            msg.append(".");
            throw webstrada::exception(msg);
        }
        // Multi-dot (or a member path with brackets): report the root variable.
        string root = nm.left(dot).trimmed();
        root.toUpper();
        throw webstrada::exception(string("Variable ") + root + " is undefined.");
    }

    string root = name;
    root.toUpper();
    throw webstrada::exception(string("Variable ") + root + " is undefined.");
}

// Shared memoization logic for the compile-time-bound fast path. On a
// successful lookup the resolved value is cached into the slot ONLY when it is
// a direct single-part member of the variables scope (a query column, an
// implicit-scope hit, a UDF parent value or a function handle are never
// cached). The cached pointer is valid while the variables scope's generation
// is unchanged (StructDelete/StructClear bump it), so a fast read never
// dereferences a freed map node.
static void maybeMemoizeVarSlot(VarFastSlot *slot, const cfvariant *v,
                                const cfvariant *variables, const char *name)
{
    if (!slot || !v || !variables) return;
    if (variables->m_type != cfvariant::Struct) return;
    StructData *sd = variables->m_structData;
    if (!sd) return;
    // Single-part name only; a dotted name resolves into a nested struct whose
    // lifetime is not guarded by the variables scope's generation counter.
    const char *p = name;
    for (; *p; ++p) {
        if (*p == '.' || *p == '[' || *p == '(') return;
    }
    webstrada::string key(name);
    key.toUpper();
    auto it = sd->map.find(key);
    if (it != sd->map.end() && &it->second == v) {
        slot->ptr = &it->second;
        slot->sd = sd;
        slot->gen = sd->generation;
    }
}

const cfvariant *cfml::cfvariant_get_var_fast(
    VarFastSlot *slot,
    const cfvariant *cgi, const cfvariant *server,
    const cfvariant *cookie, const cfvariant *application,
    const cfvariant *session, const cfvariant *url,
    const cfvariant *form, const cfvariant *variables,
    const char *name)
{
    // Fast path: the memoized pointer is a direct variables-scope member whose
    // node is still alive (generation matches) and the value is defined. Inside
    // a <cfloop query> the query scope shadows the variables scope for
    // unqualified names, so the cache is bypassed there (verified behavior).
    if (slot) {
        StructData *sd = variables && variables->m_type == cfvariant::Struct
                             ? variables->m_structData : nullptr;
        if (g_queryScopes.empty() && slot->ptr && sd &&
            slot->sd == sd && slot->gen == sd->generation &&
            slot->ptr->m_type != cfvariant::NotSet) {
            return slot->ptr;
        }
    }
    const cfvariant *v = cfvariant_get_var(cgi, server, cookie, application,
                                           session, url, form, variables, name);
    maybeMemoizeVarSlot(slot, v, variables, name);
    return v;
}

cfvariant *cfml::cfvariant_bare_identifier_fast(
    VarFastSlot *slot,
    const cfvariant *cgi, const cfvariant *server,
    const cfvariant *cookie, const cfvariant *application,
    const cfvariant *session, const cfvariant *url,
    const cfvariant *form, const cfvariant *variables,
    const char *name)
{
    if (slot) {
        StructData *sd = variables && variables->m_type == cfvariant::Struct
                             ? variables->m_structData : nullptr;
        if (g_queryScopes.empty() && slot->ptr && sd &&
            slot->sd == sd && slot->gen == sd->generation &&
            slot->ptr->m_type != cfvariant::NotSet) {
            return const_cast<cfvariant*>(slot->ptr);
        }
    }
    cfvariant *v = cfvariant_bare_identifier(cgi, server, cookie, application,
                                             session, url, form, variables, name);
    maybeMemoizeVarSlot(slot, v, variables, name);
    return v;
}

