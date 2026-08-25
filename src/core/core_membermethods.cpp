#include "core_internal.h"
#include "../cftags/common.h"

#include <webstrada/cf8.h>
#include <webstrada/component.h>
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

#include <cstdlib>
#include <cstdio>
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

// Registers a freshly-allocated result with the request's temp-variant cleanup
// (g_temp_variants, drained by cfvariant_cleanup_restore) and returns it. Only
// ever used on fresh heap allocations — never on borrowed pointers into live
// containers (scopes/arrays/structs), which would be double-freed at cleanup.
// The JIT's emitCall whitelist (isTempVariantFunction) does not register the
// cfvariant_* helpers, so registering here is the single ownership point for
// their results regardless of caller (JIT or interpreter).
static cfvariant *tempRet(cfvariant *v) {
    cf_register_temp(v);
    return v;
}

// Built-in function dispatch for cfvariant_call_function (defined below).
// Every return in it is a fresh heap allocation that the caller registers via
// tempRet; the caller must never register the UDF/component-method results that
// cfvariant_call_function resolves itself (those are already owned by the
// callee's request-cleanup).
static cfvariant *cf_call_builtin_dispatch(
    string &out, void *cgi, void *server, void *cookie, void *application,
    void *session, void *url, void *form, void *variables,
    const char *name, const string &fname, const cfvariant **args, int arg_count);
// ---- Member-method dispatch ----
// ============================================================================
// Built-in member-method dispatch (base.method(args)).
//
// CFML allows calling a built-in function as a member method of a compatible
// value: `"hi".toUpperCase()`, `arr.sort()`, `st.keyExists("k")`, `q.addRow()`.
// The method name maps to a standalone function with the receiver prepended as
// the first argument. A UDF/closure stored in a struct/xml member shadows the
// built-in table (s.member() invokes the stored callable).
// ============================================================================

struct MemberMethodEntry {
    const char *method;   // upper-cased member-method name
    const char *function; // standalone function it maps to (receiver first)
};

static const MemberMethodEntry kStringMemberMethods[] = {
    {"ASC", "ASC"},
    {"CJUSTIFY", "CJUSTIFY"},
    {"COMPARE", "COMPARE"},
    {"COMPARENOCASE", "COMPARENOCASE"},
    {"DECODEFROMURL", "DECODEFROMURL"},
    {"DECRYPT", "DECRYPT"},
    {"ENCODEFORURL", "ENCODEFORURL"},
    {"ENCRYPT", "ENCRYPT"},
    {"FIND", "FIND"},
    {"FINDNOCASE", "FINDNOCASE"},
    {"FIX", "FIX"},
    {"GETTOKEN", "GETTOKEN"},
    {"HASH", "HASH"},
    {"HTMLCODEFORMAT", "HTMLCODEFORMAT"},
    {"HTMLEDITFORMAT", "HTMLEDITFORMAT"},
    {"INT", "INT"},
    {"ISBOOLEAN", "ISBOOLEAN"},
    {"ISDATE", "ISDATE"},
    {"ISNULL", "ISNULL"},
    {"ISNUMERIC", "ISNUMERIC"},
    {"ISSIMPLEVALUE", "ISSIMPLEVALUE"},
    {"JSSTRINGFORMAT", "JSSTRINGFORMAT"},
    {"LCASE", "LCASE"},
    {"LEFT", "LEFT"},
    {"LEN", "LEN"},
    {"LISTAPPEND", "LISTAPPEND"},
    {"LISTCHANGEDELIMS", "LISTCHANGEDELIMS"},
    {"LISTCONTAINS", "LISTCONTAINS"},
    {"LISTCONTAINSNOCASE", "LISTCONTAINSNOCASE"},
    {"LISTDELETEAT", "LISTDELETEAT"},
    {"LISTFIND", "LISTFIND"},
    {"LISTFINDNOCASE", "LISTFINDNOCASE"},
    {"LISTFIRST", "LISTFIRST"},
    {"LISTGETAT", "LISTGETAT"},
    {"LISTINSERTAT", "LISTINSERTAT"},
    {"LISTLAST", "LISTLAST"},
    {"LISTLEN", "LISTLEN"},
    {"LISTPREPEND", "LISTPREPEND"},
    {"LISTQUALIFY", "LISTQUALIFY"},
    {"LISTREMOVEDUPLICATES", "LISTREMOVEDUPLICATES"},
    {"LISTREST", "LISTREST"},
    {"LISTSETAT", "LISTSETAT"},
    {"LISTSORT", "LISTSORT"},
    {"LISTTOARRAY", "LISTTOARRAY"},
    {"LISTVALUECOUNT", "LISTVALUECOUNT"},
    {"LISTVALUECOUNTNOCASE", "LISTVALUECOUNTNOCASE"},
    {"LJUSTIFY", "LJUSTIFY"},
    {"LTRIM", "LTRIM"},
    {"MID", "MID"},
    {"PARAGRAPHFORMAT", "PARAGRAPHFORMAT"},
    {"REESCAPE", "REESCAPE"},
    {"REMOVECHARS", "REMOVECHARS"},
    {"REPEATSTRING", "REPEATSTRING"},
    {"REPLACE", "REPLACE"},
    {"REPLACELIST", "REPLACELIST"},
    {"REPLACENOCASE", "REPLACENOCASE"},
    {"REVERSE", "REVERSE"},
    {"RIGHT", "RIGHT"},
    {"RJUSTIFY", "RJUSTIFY"},
    {"RTRIM", "RTRIM"},
    {"SPANEXCLUDING", "SPANEXCLUDING"},
    {"SPANINCLUDING", "SPANINCLUDING"},
    {"STRIPCR", "STRIPCR"},
    {"TOBASE64", "TOBASE64"},
    {"TOLOWERCASE", "LCASE"},
    {"TOSTRING", "TOSTRING"},
    {"TOUPPERCASE", "UCASE"},
    {"TRIM", "TRIM"},
    {"UCASE", "UCASE"},
    {"URLDECODE", "URLDECODE"},
    {"URLENCODEDFORMAT", "URLENCODEDFORMAT"},
    {"VAL", "VAL"},
    {"WRAP", "WRAP"},
    {"XMLFORMAT", "XMLFORMAT"},
};

static const MemberMethodEntry kArrayMemberMethods[] = {
    {"APPEND", "ARRAYAPPEND"},
    {"AVG", "ARRAYAVG"},
    {"AVERAGE", "ARRAYAVG"},
    {"CLEAR", "ARRAYCLEAR"},
    {"CONTAINS", "ARRAYCONTAINS"},
    {"CONTAINSNOCASE", "ARRAYCONTAINSNOCASE"},
    {"DELETE", "ARRAYDELETE"},
    {"DELETEAT", "ARRAYDELETEAT"},
    {"DELETENOCASE", "ARRAYDELETENOCASE"},
    {"EACH", "ARRAYEACH"},
    {"FILTER", "ARRAYFILTER"},
    {"FIND", "ARRAYFIND"},
    {"FINDALL", "ARRAYFINDALL"},
    {"FINDALLNOCASE", "ARRAYFINDALLNOCASE"},
    {"FINDNOCASE", "ARRAYFINDNOCASE"},
    {"FIRST", "ARRAYFIRST"},
    {"INSERTAT", "ARRAYINSERTAT"},
    {"ISDEFINED", "ARRAYISDEFINED"},
    {"ISEMPTY", "ARRAYISEMPTY"},
    {"LAST", "ARRAYLAST"},
    {"LEN", "ARRAYLEN"},
    {"MAP", "ARRAYMAP"},
    {"MAX", "ARRAYMAX"},
    {"MIN", "ARRAYMIN"},
    {"POP", "ARRAYPOP"},
    {"PREPEND", "ARRAYPREPEND"},
    {"PUSH", "ARRAYAPPEND"},
    {"REDUCE", "ARRAYREDUCE"},
    {"RESIZE", "ARRAYRESIZE"},
    {"SET", "ARRAYSET"},
    {"SHIFT", "ARRAYSHIFT"},
    {"SIZE", "ARRAYLEN"},
    {"SLICE", "ARRAYSLICE"},
    {"SORT", "ARRAYSORT"},
    {"SUM", "ARRAYSUM"},
    {"SWAP", "ARRAYSWAP"},
    {"TOLIST", "ARRAYTOLIST"},
    {"UNSHIFT", "ARRAYPREPEND"},
};

static const MemberMethodEntry kStructMemberMethods[] = {
    {"APPEND", "STRUCTAPPEND"},
    {"CLEAR", "STRUCTCLEAR"},
    {"COPY", "STRUCTCOPY"},
    {"COUNT", "STRUCTCOUNT"},
    {"DELETE", "STRUCTDELETE"},
    {"EACH", "STRUCTEACH"},
    {"FILTER", "STRUCTFILTER"},
    {"FIND", "STRUCTFIND"},
    {"FINDKEY", "STRUCTFINDKEY"},
    {"FINDVALUE", "STRUCTFINDVALUE"},
    {"GETMETADATA", "STRUCTGETMETADATA"},
    {"INSERT", "STRUCTINSERT"},
    {"ISEMPTY", "STRUCTISEMPTY"},
    {"KEYARRAY", "STRUCTKEYARRAY"},
    {"KEYEXISTS", "STRUCTKEYEXISTS"},
    {"KEYLIST", "STRUCTKEYLIST"},
    {"LEN", "STRUCTCOUNT"},
    {"MAP", "STRUCTMAP"},
    {"REDUCE", "STRUCTREDUCE"},
    {"SORT", "STRUCTSORT"},
    {"UPDATE", "STRUCTUPDATE"},
    {"VALUEARRAY", "STRUCTVALUEARRAY"},
};

static const MemberMethodEntry kQueryMemberMethods[] = {
    {"ADDCOLUMN", "QUERYADDCOLUMN"},
    {"ADDROW", "QUERYADDROW"},
    {"GETMETADATA", "GETMETADATA"},
    {"GETRESULT", "QUERYGETRESULT"},
    {"GETROW", "QUERYGETROW"},
    {"KEYEXISTS", "QUERYKEYEXISTS"},
    {"LEN", "QUERYGETRESULT"},
};

static const MemberMethodEntry kDateMemberMethods[] = {
    {"ADD", "DATEADD"},
    {"COMPARE", "DATECOMPARE"},
    {"CONVERT", "DATECONVERT"},
    {"DATEDIFF", "DATEDIFF"},
    {"DATEFORMAT", "DATEFORMAT"},
    {"DATEPART", "DATEPART"},
    {"DATETIMEFORMAT", "DATETIMEFORMAT"},
    {"DAY", "DAY"},
    {"DAYOFWEEK", "DAYOFWEEK"},
    {"DAYOFWEEKASSTRING", "DAYOFWEEKASSTRING"},
    {"DAYOFYEAR", "DAYOFYEAR"},
    {"DAYSINMONTH", "DAYSINMONTH"},
    {"DAYSINYEAR", "DAYSINYEAR"},
    {"FIRSTDAYOFMONTH", "FIRSTDAYOFMONTH"},
    {"HOUR", "HOUR"},
    {"ISLEAPYEAR", "ISLEAPYEAR"},
    {"MINUTE", "MINUTE"},
    {"MONTH", "MONTH"},
    {"MONTHASSTRING", "MONTHASSTRING"},
    {"QUARTER", "QUARTER"},
    {"SECOND", "SECOND"},
    {"SETDAY", "SETDAY"},
    {"SETHOUR", "SETHOUR"},
    {"SETMINUTE", "SETMINUTE"},
    {"SETMONTH", "SETMONTH"},
    {"SETSECOND", "SETSECOND"},
    {"SETYEAR", "SETYEAR"},
    {"TIMEFORMAT", "TIMEFORMAT"},
    {"WEEK", "WEEK"},
    {"YEAR", "YEAR"},
};

static const MemberMethodEntry kXmlMemberMethods[] = {
    {"GETNODETYPE", "XMLGETNODETYPE"},
    {"SEARCH", "XMLSEARCH"},
};

// Maps a member-method name (upper-cased) to the standalone function that
// implements it for the receiver's type, or nullptr when the receiver type has
// no such member method.
static const char *mapMemberMethod(cfvariant::cfvariantType type, const string &upper)
{
    const MemberMethodEntry *table = nullptr;
    size_t count = 0;
    switch (type) {
        case cfvariant::Array:     table = kArrayMemberMethods;  count = sizeof(kArrayMemberMethods)  / sizeof(kArrayMemberMethods[0]);  break;
        case cfvariant::Struct:    table = kStructMemberMethods; count = sizeof(kStructMemberMethods) / sizeof(kStructMemberMethods[0]); break;
        case cfvariant::Query:     table = kQueryMemberMethods;  count = sizeof(kQueryMemberMethods)  / sizeof(kQueryMemberMethods[0]);  break;
        case cfvariant::DateTime:  table = kDateMemberMethods;   count = sizeof(kDateMemberMethods)   / sizeof(kDateMemberMethods[0]);   break;
        case cfvariant::Xml:       table = kXmlMemberMethods;    count = sizeof(kXmlMemberMethods)    / sizeof(kXmlMemberMethods[0]);    break;
        default:                   table = kStringMemberMethods; count = sizeof(kStringMemberMethods) / sizeof(kStringMemberMethods[0]); break;
    }
    for (size_t i = 0; i < count; i++) {
        if (upper.equals(table[i].method)) return table[i].function;
    }
    return nullptr;
}

static const char *variantTypeName(cfvariant::cfvariantType type)
{
    switch (type) {
        case cfvariant::Array:    return "array";
        case cfvariant::Struct:   return "struct";
        case cfvariant::Query:    return "query";
        case cfvariant::Xml:      return "XML";
        case cfvariant::DateTime: return "date";
        case cfvariant::String:   return "string";
        default:                  return "value";
    }
}

// Full member-method resolution + dispatch, shared by the JIT runtime symbol,
// the cfoutput interpreter (applyMemberChain) and the interpreter's function
// call path. `base` is the receiver (mutating methods write in place); `args`
// holds the already-evaluated call arguments (without the receiver).
static bool memberMethodDiagnosticsEnabled()
{
    const char *value = std::getenv("WEBSTRADA_DEBUG_MEMBER_METHODS");
    return value && value[0] && value[0] != '0';
}

cfvariant invokeMemberMethod(
    cfvariant &base, const string &methodName,
    const cfvariant **args, int arg_count,
    string &out, void *cgi, void *server, void *cookie, void *application,
    void *session, void *url, void *form, void *variables)
{
    string upper = methodName;
    upper.toUpper();

    if (memberMethodDiagnosticsEnabled()) {
        std::fprintf(stderr,
                     "[WebStrada][MemberDebug] method=%s baseType=%d component=%p superTarget=%p struct=%p\n",
                     methodName.constData() ? methodName.constData() : "",
                     static_cast<int>(base.m_type),
                     static_cast<void *>(base.m_component),
                     static_cast<void *>(base.m_superTargetInfo),
                     static_cast<void *>(base.m_struct));
        if (base.m_type == cfvariant::Component && base.m_component && base.m_component->info) {
            const ComponentInfo *info = base.m_component->info;
            std::fprintf(stderr,
                         "[WebStrada][MemberDebug] component name=%s path=%s cfc=%s methods=%zu\n",
                         info->name.c_str(), info->path.c_str(), info->cfcPath.c_str(), info->methods.size());
            for (const ComponentInfo *cur = info; cur; cur = cur->parent) {
                std::fprintf(stderr, "[WebStrada][MemberDebug] methodTable=%s:", cur->name.c_str());
                for (const auto &method : cur->methods) std::fprintf(stderr, " %s(%s)", method.name.c_str(), method.access.c_str());
                std::fprintf(stderr, "\n");
            }
        } else if (base.m_type == cfvariant::Struct && base.m_struct) {
            std::fprintf(stderr, "[WebStrada][MemberDebug] structKeys:");
            for (const auto &entry : *base.m_struct) {
                std::fprintf(stderr, " %s(type=%d)", entry.first.constData() ? entry.first.constData() : "",
                             static_cast<int>(entry.second.m_type));
            }
            std::fprintf(stderr, "\n");
        }
    }

    // 0. Component method dispatch: obj.method(args). A this-scope Function
    //    value (a UDF stored in the this scope) wins; otherwise the method is
    //    looked up in the component's method table with CF access control.
    if (base.m_type == cfvariant::Component) {
        cfvariant *res = cfml::cf_component_invoke(&base, methodName.constData(), args, arg_count,
                                                   out, cgi, server, cookie, application, session, url, form);
        return *res;
    }

    // 1. A callable (UDF/closure) stored in a struct/xml member wins over the
    //    built-in member-method table (CF: s.member() invokes the stored UDF).
    //    The key must already exist — member lookup never auto-creates.
    if (base.m_type == cfvariant::Struct || base.m_type == cfvariant::Xml) {
        auto it = base.m_struct->find(upper);
        if (it != base.m_struct->end()) {
            cfvariant &member = it->second;
            if (member.m_type == cfvariant::Function && member.m_udf && member.m_udf->fn) {
                cfvariant *res = cfml::cf_udf_invoke(&member, args, arg_count,
                                                     out, cgi, server, cookie, application, session, url, form, variables);
                return *res;
            }
            throw webstrada::exception("Entity has incorrect type for being called as a function.");
        }
    }

    // 2. Query pseudo-member: `q.len()` returns the record count (matches CF).
    if (base.m_type == cfvariant::Query && upper.equals("LEN")) {
        return cfvariant(queryRecordCount(&base));
    }

    // A query-column reference is not a real array; its member methods follow
    // CF's scalar-first-cell QueryColumn semantics. In particular x.len()
    // returns the first cell's length (CF 2021: Len(x) == 1 for a varchar
    // column whose first cell is "x"), not the ArrayLen rejection.
    if (isQueryColumnRef(&base) && upper.equals("LEN")) {
        cfvariant *lenTmp = cfml::cf_len(&base);
        cf_register_temp(lenTmp);
        return *lenTmp;
    }

    // 3. Built-in member-method table: dispatch with the receiver prepended.
    //    Most member methods take the receiver as the standalone function's
    //    first argument, but the date date-methods DateDiff/DateAdd/DatePart/
    //    DateConvert place it elsewhere (dateDiff(part, date1, date2),
    //    dateAdd(part, n, date), datePart(part, date), dateConvert(type, date)).
    //    `push` is special: CF's arr.push(x) returns the array's NEW LENGTH
    //    (verified on the RDS host), while ArrayAppend/append return YES.
    if (upper.equals("PUSH") && base.m_type == cfvariant::Array && arg_count == 1) {
        if (base.m_isXmlNodeList) throwXmlNodeListUnsupported("ArrayPush");
        base.insert(*args[0]);
        return cfvariant(static_cast<int>(base.m_array->size()));
    }
    const char *fnName = mapMemberMethod(base.m_type, upper);
    if (fnName) {
        string fnUpper(fnName);
        fnUpper.toUpper();
        if (!isKnownFunctionName(fnUpper)) {
            throw webstrada::exception(string("Function ") + fnUpper + " is not implemented");
        }
        std::vector<const cfvariant*> callArgs;
        if (upper.equals("DATEDIFF") && arg_count == 2) {
            callArgs.push_back(args[0]); callArgs.push_back(&base); callArgs.push_back(args[1]);
        } else if (upper.equals("ADD") && arg_count == 2) {
            callArgs.push_back(args[0]); callArgs.push_back(args[1]); callArgs.push_back(&base);
        } else if ((upper.equals("DATEPART") || upper.equals("CONVERT")) && arg_count == 1) {
            callArgs.push_back(args[0]); callArgs.push_back(&base);
        } else if ((upper.equals("FIND") || upper.equals("FINDNOCASE")) && arg_count == 1 &&
                   base.m_type != cfvariant::Array && base.m_type != cfvariant::Struct &&
                   base.m_type != cfvariant::Query && base.m_type != cfvariant::Xml) {
            callArgs.push_back(args[0]); callArgs.push_back(&base);
        } else {
            callArgs.push_back(&base);
            for (int i = 0; i < arg_count; i++) callArgs.push_back(args[i]);
        }
        cfvariant *res = cfml::cfvariant_call_function(
            out, cgi, server, cookie, application, session, url, form, variables,
            fnUpper.constData(), callArgs.data(), static_cast<int>(callArgs.size()));
        if (base.m_type == cfvariant::DateTime &&
            (upper.equals("SETYEAR") || upper.equals("SETMONTH") || upper.equals("SETDAY") ||
             upper.equals("SETHOUR") || upper.equals("SETMINUTE") || upper.equals("SETSECOND"))) {
            base.m_double = res->m_double;
            base.m_type = res->m_type;
        }
        return *res;
    }

    // 4. No such member/method on this receiver.
    // CF reports type "Object" with "The <method> method was not found."
    // (verified on CF 2025: arr.nosuch() / s.foo() -> [Object]The nosuch method
    // was not found.; the method name keeps its original case).
    webstrada::string msg("The ");
    msg.append(methodName);
    msg.append(" method was not found.");
    throw webstrada::exception("Object", msg, "");
}

cfvariant *cfml::cfvariant_member_method(
    cfvariant *base, const char *name,
    const cfvariant **args, int arg_count,
    string &out, void *cgi, void *server, void *cookie, void *application,
    void *session, void *url, void *form, void *variables)
{
    if (!base) throw webstrada::exception("Member method called on an undefined value");
    cfvariant res = invokeMemberMethod(*base, string(name), args, arg_count,
        out, cgi, server, cookie, application, session, url, form, variables);
    auto *ret = new cfvariant(res);
    return tempRet(ret);
}

// Whole-column assignment q.a = v (dot access). CF 2021 writes the value into
// the current row's cell rather than replacing the column (verified on the RDS
// host: `q.a = "whole"` sets only `q.a[1]`). A non-column name throws CF's
// "columnMap" Application error; a complex RHS (array/struct/query/...) and a
// deeper dotted chain (q.a.b = v) are rejected with CF's "trying to modify the
// query" Expression error — CF's own array-to-column behavior is erratic
// (sometimes silently accepted, sometimes an Expression error, sometimes a
// page abort), so it is deliberately not replicated (see BUGS.md).
static cfvariant *assignQueryColumn(cfvariant *query, const string &memberPath, const cfvariant *value)
{
    string upper = memberPath;
    upper.toUpper();
    if (upper.indexOf('.') >= 0) {
        throw webstrada::exception("An error occurred while trying to modify the query named class coldfusion.sql.QueryTable.");
    }
    int colIdx = query->m_query->findColumn(upper);
    if (colIdx < 0) {
        throw webstrada::exception("Application",
            "There is a problem in the column mappings specified in the columnMap structure.",
            "The query attribute input does not contain any column by the name of " + memberPath + ".");
    }
    if (value->m_type == cfvariant::Array || value->m_type == cfvariant::Struct ||
        value->m_type == cfvariant::Xml || value->m_type == cfvariant::JSon ||
        value->m_type == cfvariant::Query || value->m_type == cfvariant::Component ||
        value->m_type == cfvariant::Binary || value->m_type == cfvariant::Image ||
        value->m_type == cfvariant::Function) {
        throw webstrada::exception("An error occurred while trying to modify the query named class coldfusion.sql.QueryTable.");
    }
    QueryColumn &col = query->m_query->columns[colIdx];
    int row = query->m_query->currentRow;
    if (row < 1) row = 1;
    if (row > (int)col.values.size()) col.values.resize(row, cfvariant(cfvariant::Null));
    col.values[row - 1] = coerceQueryCell(col.type, *value);
    auto *res = new cfvariant(*value);
    return tempRet(res);
}

cfvariant *cfml::cfvariant_assign(
    const cfvariant *cgi, const cfvariant *server,
    const cfvariant *cookie, const cfvariant *application,
    const cfvariant *session, const cfvariant *url,
    const cfvariant *form, cfvariant *variables,
    const char *name, const cfvariant *value)
{
    cfvariant *scope = variables;
    string varName(name);

    int dotPos = varName.indexOf('.');
    if (dotPos > 0) {
        string scopeName = varName.left(dotPos).trimmed();
        string scopeNameOrig = scopeName;
        varName = varName.mid(dotPos + 1, varName.length() - dotPos - 1).trimmed();
        scopeName.toUpper();

        if (scopeName.equals("VARIABLES"))       scope = const_cast<cfvariant*>(udfVariablesScope(variables));
        else if (scopeName.equals("FORM"))       scope = const_cast<cfvariant*>(form);
        else if (scopeName.equals("URL"))        scope = const_cast<cfvariant*>(url);
        else if (scopeName.equals("CGI"))        scope = const_cast<cfvariant*>(cgi);
        else if (scopeName.equals("SERVER"))     scope = const_cast<cfvariant*>(server);
        else if (scopeName.equals("COOKIE"))     scope = const_cast<cfvariant*>(cookie);
        else if (scopeName.equals("APPLICATION")) scope = const_cast<cfvariant*>(application);
        else if (scopeName.equals("SESSION"))    scope = const_cast<cfvariant*>(session);
        else if (scopeName.equals("REQUEST"))    scope = &g_requestScope;
        else if (scopeName.equals("CALLER")) {
            if (g_customTagStack.empty() || !g_customTagStack.back().callerVariables) {
                throw webstrada::exception("Variable CALLER is undefined.");
            }
            scope = g_customTagStack.back().callerVariables;
        } else if (scopeName.equals("ATTRIBUTES")) {
            if (g_customTagStack.empty()) throw webstrada::exception("Variable ATTRIBUTES is undefined.");
            scope = &g_customTagStack.back().attributes;
        } else if (scopeName.equals("THISTAG")) {
            if (g_customTagStack.empty()) throw webstrada::exception("Variable THISTAG is undefined.");
            scope = &g_customTagStack.back().thisTag;
            // thisTag.executionMode / hasEndTag are read-only in CF
            // (ThistagScope rejects them); assigning generatedContent marks the
            // custom-tag runtime so the end-mode output logic replaces the
            // captured body with the new value.
            string firstSeg = varName;
            int segDot = firstSeg.indexOf('.');
            if (segDot >= 0) firstSeg = firstSeg.left(segDot);
            string firstSegUp = firstSeg;
            firstSegUp.toUpper();
            if (firstSegUp.equals("HASENDTAG")) {
                throw webstrada::exception("Expression", "The HasEndTag variable in the ThisTag scope cannot be set by the user.", "");
            }
            if (firstSegUp.equals("EXECUTIONMODE")) {
                throw webstrada::exception("Expression", "The ExecutionMode variable in the ThisTag scope cannot be set by the user.", "");
            }
            if (firstSegUp.equals("GENERATEDCONTENT")) {
                cfml::cf_custom_tag_mark_content_changed();
            }
        } else if (scopeName.equals("THIS")) {
            // this.x = v writes the component's this scope (only valid inside
            // a component method/body; CF: page-level `this` is undefined).
            // ColdFusion stores this-scope keys uppercased.
            bool found = false;
            for (auto it = g_udfCtx.rbegin(); it != g_udfCtx.rend(); ++it) {
                if (it->thisScope) { scope = it->thisScope; found = true; break; }
            }
            if (!found) throw webstrada::exception("Variable THIS is undefined.");
            varName.toUpper();
        } else if (scopeName.equals("LOCAL")) {
            if (g_udfCtx.empty()) throw webstrada::exception("Variable LOCAL is undefined.");
            scope = g_udfCtx.back().localScope;
        } else {
            cfvariant *targetStruct = lookupVarWritable(scopeName.constData(),
                const_cast<cfvariant*>(cgi),
                const_cast<cfvariant*>(server),
                const_cast<cfvariant*>(cookie),
                const_cast<cfvariant*>(application),
                const_cast<cfvariant*>(session),
                const_cast<cfvariant*>(url),
                const_cast<cfvariant*>(form),
                const_cast<cfvariant*>(variables));
            // The dotted prefix may be a query variable: q.a = v is a
            // whole-column assignment that writes the current row's cell
            // (CF 2021, verified on the RDS host).
            if (targetStruct && targetStruct->m_type == cfvariant::Query && targetStruct->m_query) {
                return assignQueryColumn(targetStruct, varName, value);
            }
            if (targetStruct && targetStruct->m_type == cfvariant::Struct) {
                scope = targetStruct;
            } else if (targetStruct && targetStruct->m_type == cfvariant::Component) {
                // o.member = v writes the component's this scope (keys
                // uppercased like CF's this scope).
                scope = targetStruct;
                varName.toUpper();
            } else {
                // The dotted prefix is not a scope and not an existing
                // variable: CF auto-creates the whole dotted path in the
                // variables scope (SetVariable("a.b.c", v) -> variables.a.b.c).
                // Restore the consumed prefix so the walk below creates it.
                scope = variables;
                varName = scopeNameOrig + "." + varName;
            }
        }
    } else if (g_customTagExecutionVariables) {
        scope = g_customTagExecutionVariables;
    } else if (!g_udfCtx.empty()) {
        // Inside a UDF, an unqualified assignment targets the captured parent
        // scope unless the name is a local (var / nested function /
        // arguments). This matches CF's classic UDF scope leak for non-var
        // names (verified on CF 2021). A parameter name, however, writes to the
        // `arguments` scope — even when a `local.arg1` was explicitly created
        // (CF: `local.arg1 = "L"; arg1 = "Y"` keeps local.arg1 == "L").
        scope = udfAssignScope(variables, name);
    } else if (auto *rt = cfml::include_context(); rt && rt->includeLocalScope) {
        // Unqualified writes in an included template target the caller's
        // local scope when that caller is a function, while explicit
        // `variables.foo` still uses the variables argument passed above.
        scope = rt->includeLocalScope;
    }

    if (!scope || (scope->m_type != cfvariant::Struct && scope->m_type != cfvariant::Component)) {
        throw webstrada::exception("Cannot assign variable: target scope is not a valid structure.");
    }

    // Walk any remaining dotted path (a.b.c = v). The first segment resolved to
    // `scope`; the rest are struct members that are auto-created when absent
    // (matching CF) and must be structs to continue (CF: "You have attempted to
    // dereference a scalar variable ... as a structure with members."). A
    // Component intermediate is followed into its this scope (arguments.event.x
    // = v writes the event object's this scope, verified against CF 2025: the
    // Colorer plugin's `arguments.event.outputData = data`).
    cfvariant *target = scope;
    string rest = varName;
    while (true) {
        int d = rest.indexOf('.');
        if (d < 0) break;
        string key = rest.left(d).trimmed();
        rest = rest.mid(d + 1, rest.length() - d - 1).trimmed();
        if (!target || (target->m_type != cfvariant::Struct && target->m_type != cfvariant::Xml &&
                        target->m_type != cfvariant::Component)) {
            throw webstrada::exception("You have attempted to dereference a scalar variable as a structure with members.");
        }
        // Component members live in the this scope with uppercased keys.
        if (target->m_type == cfvariant::Component) key.toUpper();
        cfvariant &member = target->set(key);
        if (member.m_type != cfvariant::Struct && member.m_type != cfvariant::Xml &&
            member.m_type != cfvariant::Component && member.m_type != cfvariant::NotSet) {
            throw webstrada::exception("You have attempted to dereference a scalar variable as a structure with members.");
        }
        if (member.m_type == cfvariant::NotSet) {
            member.set_type(cfvariant::Struct);
        }
        target = &member;
    }

    // Preserve original key casing; lookups are case-insensitive via CiLess.
    // A Component target writes the final member into its this scope (uppercased).
    if (target->m_type == cfvariant::Component) rest.toUpper();
    cfvariant &slot = target->set(rest);

    if (value->m_type == cfvariant::File) {
        // Close old slot if it held a file (safe: FileClose sets
        // m_fd to -1, so a reopened fd number is never stale)
        if (slot.m_type == cfvariant::File && slot.m_fd > 2)
            close(slot.m_fd);
        else if (slot.m_type != cfvariant::NotSet)
            slot.set_type(cfvariant::NotSet);

        // Steal fd from value (transfer, not copy)
        auto *val = const_cast<cfvariant*>(value);
        slot.m_type = cfvariant::File;
        slot.m_fd = val->m_fd;
        val->m_type = cfvariant::NotSet;

        // Return value: dup from slot so the expression result is valid
        auto *res = new cfvariant(cfvariant::File);
        res->m_fd = slot.m_fd > 2 ? dup(slot.m_fd) : slot.m_fd;
        return tempRet(res);
    }

    slot = *value;
    storeQueryColumnRef(slot);

    auto *res = new cfvariant(*value);
    return tempRet(res);
}

// Numeric coercion for arithmetic operands. A query-column reference
// (q.price) acts as its current row's scalar cell, and a numeric string
// ("12.5") coerces to a number like CF's Arith (verified: "12.5" + 1 -> 13.5,
// "2" + 1 -> 3, "abc" + 1 throws "The value abc cannot be converted to a
// number."). Returns whether the operand should take the double path.
static bool arithOperand(const cfvariant **inOut, cfvariant &scalar, double &num)
{
    const cfvariant *v = *inOut;
    scalar = scalarizeQueryColumn(v);
    *inOut = &scalar;
    const cfvariant &cell = scalar;
    if (cell.m_type == cfvariant::Float) {
        num = cell.m_double;
        return true;
    }
    if (cell.m_type == cfvariant::Number || cell.m_type == cfvariant::Long ||
        cell.m_type == cfvariant::Boolean) {
        return false;
    }
    // String (or a scalarized cell holding one): parse as a number. CF's
    // error is "The value X cannot be converted to a number." (no quotes).
    if (cell.m_type == cfvariant::String) {
        string s = const_cast<cfvariant&>(cell).toString();
        std::string ss(s.constData() ? s.constData() : "");
        const char *str = ss.c_str();
        if (!str) throw webstrada::exception("The value  cannot be converted to a number.");
        const char *p = str;
        while (*p && isspace(*p)) p++;
        if (*p == '\0') throw webstrada::exception("The value  cannot be converted to a number.");
        char *end = nullptr;
        double d = strtod(p, &end);
        while (end && *end && isspace(*end)) end++;
        if (end == p || (end && *end != '\0')) {
            throw webstrada::exception(("The value " + ss + " cannot be converted to a number.").c_str());
        }
        // A pure integer string stays on the integer path ("2" + 1 -> 3);
        // anything with a fraction/exponent takes the double path.
        char *iend = nullptr;
        long long iv = strtoll(p, &iend, 10);
        while (iend && *iend && isspace(*iend)) iend++;
        bool pureInt = (iend && *iend == '\0' && d == static_cast<double>(iv) &&
                        ss.find('.') == std::string::npos &&
                        ss.find('e') == std::string::npos &&
                        ss.find('E') == std::string::npos);
        num = d;
        return !pureInt;
    }
    // DateTime / other: leave to getLongIntValue/getDoubleValue semantics.
    return cell.m_type == cfvariant::DateTime;
}

cfvariant *cfml::cfvariant_add(const cfvariant *a, const cfvariant *b) {
    cfvariant *v = nullptr;
    cfvariant sa, sb;
    double da = 0.0, db = 0.0;
    bool aFloat = arithOperand(&a, sa, da);
    bool bFloat = arithOperand(&b, sb, db);
    if (aFloat || bFloat) {
        v = new cfvariant(cfvariant::Float);
        v->m_double = getDoubleValue(*a) + getDoubleValue(*b);
    } else {
        // Compute in 128-bit (exact for any two int64 operands) and promote to
        // Float on int32 overflow, matching CF (2147483647 + 1 -> 2147483648,
        // 1000000000*1000000000 -> 1E+018).
        __int128 res = (__int128)getLongIntValue(*a) + (__int128)getLongIntValue(*b);
        if (res >= -2147483648LL && res <= 2147483647LL) {
            v = new cfvariant(static_cast<int>(res));
        } else {
            v = new cfvariant(cfvariant::Float);
            v->m_double = static_cast<double>(res);
        }
    }
    return tempRet(v);
}

cfvariant *cfml::cfvariant_sub(const cfvariant *a, const cfvariant *b) {
    cfvariant *v = nullptr;
    cfvariant sa, sb;
    double da = 0.0, db = 0.0;
    bool aFloat = arithOperand(&a, sa, da);
    bool bFloat = arithOperand(&b, sb, db);
    if (aFloat || bFloat) {
        v = new cfvariant(cfvariant::Float);
        v->m_double = getDoubleValue(*a) - getDoubleValue(*b);
    } else {
        // See cfvariant_add: promote to Float on int32 overflow.
        __int128 res = (__int128)getLongIntValue(*a) - (__int128)getLongIntValue(*b);
        if (res >= -2147483648LL && res <= 2147483647LL) {
            v = new cfvariant(static_cast<int>(res));
        } else {
            v = new cfvariant(cfvariant::Float);
            v->m_double = static_cast<double>(res);
        }
    }
    return tempRet(v);
}

cfvariant *cfml::cfvariant_mul(const cfvariant *a, const cfvariant *b) {
    cfvariant *v = nullptr;
    cfvariant sa, sb;
    double da = 0.0, db = 0.0;
    bool aFloat = arithOperand(&a, sa, da);
    bool bFloat = arithOperand(&b, sb, db);
    if (aFloat || bFloat) {
        v = new cfvariant(cfvariant::Float);
        v->m_double = getDoubleValue(*a) * getDoubleValue(*b);
    } else {
        // See cfvariant_add: promote to Float on int32 overflow.
        __int128 res = (__int128)getLongIntValue(*a) * (__int128)getLongIntValue(*b);
        if (res >= -2147483648LL && res <= 2147483647LL) {
            v = new cfvariant(static_cast<int>(res));
        } else {
            v = new cfvariant(cfvariant::Float);
            v->m_double = static_cast<double>(res);
        }
    }
    return tempRet(v);
}

cfvariant *cfml::cfvariant_div(const cfvariant *a, const cfvariant *b) {
    double db = getDoubleValue(*b);
    if (db == 0.0) throw webstrada::exception("Division by zero.");
    cfvariant *v = new cfvariant(cfvariant::Float);
    v->m_double = getDoubleValue(*a) / db;
    return tempRet(v);
}

cfvariant *cfml::cfvariant_mod(const cfvariant *a, const cfvariant *b) {
    cfvariant sa, sb;
    double da = 0.0, db = 0.0;
    bool aFloat = arithOperand(&a, sa, da);
    bool bFloat = arithOperand(&b, sb, db);
    // MOD truncates both operands toward zero like CF's Cast._integer ("12.9"
    // MOD 3 -> 0: trunc(12.9)=12, 12 % 3 = 0).
    int ia = aFloat ? static_cast<int>(da) : getIntValue(*a);
    int ib = bFloat ? static_cast<int>(db) : getIntValue(*b);
    if (ib == 0) throw webstrada::exception("Division by zero.");
    auto *v = new cfvariant(ia % ib);
    return tempRet(v);
}

cfvariant *cfml::cfvariant_idiv(const cfvariant *a, const cfvariant *b) {
    // CF's '\' integer division truncates the floating quotient toward zero
    // (5 \ 2 -> 2, 5.9 \ 2 -> 2, -5 \ 2 -> -2). Result is an integer.
    double db = getDoubleValue(*b);
    if (db == 0.0) throw webstrada::exception("Division by zero.");
    long long iq = static_cast<long long>(getDoubleValue(*a) / db);
    cfvariant *v = nullptr;
    if (iq >= -2147483648LL && iq <= 2147483647LL) {
        v = new cfvariant(static_cast<int>(iq));
    } else {
        v = new cfvariant(cfvariant::Long);
        v->m_long = iq;
    }
    return tempRet(v);
}

cfvariant *cfml::cfvariant_pow(const cfvariant *a, const cfvariant *b) {
    // CF computes '^' in floating point (Java Math.pow semantics), so the
    // result is always a Float (2^53 cannot fit in an int32). Integral
    // results still render without a fractional part (2^3 -> 8).
    cfvariant *v = new cfvariant(cfvariant::Float);
    v->m_double = std::pow(getDoubleValue(*a), getDoubleValue(*b));
    return tempRet(v);
}

cfvariant *cfml::cfvariant_neg(const cfvariant *a) {
    cfvariant *v = nullptr;
    cfvariant sa;
    double da = 0.0;
    bool aFloat = arithOperand(&a, sa, da);
    if (aFloat) {
        v = new cfvariant(cfvariant::Float);
        v->m_double = -da;
    } else if (a->m_type == cfvariant::Long) {
        // -(Long) stays exact; within int32 the negation folds to a Number.
        __int128 res = -(__int128)a->m_long;
        if (res >= -2147483648LL && res <= 2147483647LL) {
            v = new cfvariant(static_cast<int>(res));
        } else if (res >= (__int128)LLONG_MIN && res <= (__int128)LLONG_MAX) {
            v = new cfvariant(cfvariant::Long);
            v->m_long = static_cast<long long>(res);
        } else {
            v = new cfvariant(cfvariant::Float);
            v->m_double = static_cast<double>(res);
        }
    } else if (a->m_type == cfvariant::Number) {
        v = new cfvariant(-a->m_int);
    } else {
        char *end = nullptr;
        double d = strtod(const_cast<cfvariant*>(a)->toString().constData(), &end);
        if (*end == '\0') {
            v = new cfvariant(cfvariant::Float);
            v->m_double = -d;
        } else {
            v = new cfvariant(-atoi(const_cast<cfvariant*>(a)->toString().constData()));
        }
    }
    return tempRet(v);
}

cfvariant *cfml::cfvariant_concat(const cfvariant *a, const cfvariant *b) {
    // string concatenation using '&' operator
    string sa = const_cast<cfvariant*>(a)->toString();
    string sb = const_cast<cfvariant*>(b)->toString();
    auto *v = new cfvariant(sa + sb);
    return tempRet(v);
}

cfvariant *cfml::cfvariant_and(const cfvariant *a, const cfvariant *b) {
    // CF's AND returns one of its operands rather than a fresh boolean: the
    // first operand when it is falsy, otherwise the second (verified against
    // CF 2021: true AND false -> false, blit AND (5 GT 3) -> YES). Returning
    // the operand preserves the literal/computed stringification flag.
    cfvariant *v = new cfvariant(isTruthy(*a) ? *b : *a);
    return tempRet(v);
}

cfvariant *cfml::cfvariant_or(const cfvariant *a, const cfvariant *b) {
    // CF's OR returns its first operand when truthy, otherwise the second
    // (verified against CF 2021: (5 GT 3) OR true -> YES, false OR true -> true).
    cfvariant *v = new cfvariant(isTruthy(*a) ? *a : *b);
    return tempRet(v);
}

cfvariant *cfml::cfvariant_xor(const cfvariant *a, const cfvariant *b) {
    auto *v = new cfvariant(cfvariant::Boolean);
    v->m_bool = isTruthy(*a) ^ isTruthy(*b);
    return tempRet(v);
}

cfvariant *cfml::cfvariant_not(const cfvariant *a) {
    auto *v = new cfvariant(cfvariant::Boolean);
    v->m_bool = !isTruthy(*a);
    return tempRet(v);
}

cfvariant *cfml::cfvariant_compare(const cfvariant *a, const cfvariant *b, const char *op) {
    auto *v = new cfvariant(cfvariant::Boolean);
    v->m_bool = compareVariants(a, b, op) != 0;
    return tempRet(v);
}

// CF's script `?:` ternary condition uses CfJspPage._isTruthyValue, which is a
// *different* truthiness rule than Cast._boolean / isTruthy:
//   null -> false; Boolean -> its value; Integer -> != 0; a String is false
//   only for "false"/"no"/"0" (case-insensitive) and true otherwise (including
//   the empty string); everything else (Float, Struct, Array, ...) -> true.
// Verified against CF 2025 on the RDS host. Returns the selected branch.
int cfml::cf_is_truthy_value(const cfvariant *v) {
    if (!v || v->m_type == cfvariant::Null || v->m_type == cfvariant::NotSet) return 0;
    switch (v->m_type) {
    case cfvariant::Boolean: return v->m_bool ? 1 : 0;
    case cfvariant::Number:  return v->m_int != 0 ? 1 : 0;
    case cfvariant::Long:    return v->m_long != 0 ? 1 : 0;
    case cfvariant::Float:
    case cfvariant::DateTime:
        return v->m_double != 0.0 ? 1 : 0;
    case cfvariant::String: {
        // A String is false only for "false"/"no"/"0" (case-insensitive);
        // any other string — including the empty string — is true.
        if (!v->m_str || v->m_str->isEmpty()) return 1;
        const string &s = *v->m_str;
        if (s.compareCaseInsensitive("false") == 0 ||
            s.compareCaseInsensitive("no") == 0 ||
            s.compareCaseInsensitive("0") == 0) return 0;
        return 1;
    }
    default:
        // Float, DateTime, Struct, Array, Query, Xml, Component, Binary, ... -> true.
        return 1;
    }
}

cfvariant *cfml::cf_ternary_select(const cfvariant *cond, const cfvariant *thenV, const cfvariant *elseV) {
    if (cf_is_truthy_value(cond)) return tempRet(new cfvariant(*thenV));
    return tempRet(new cfvariant(*elseV));
}

cfvariant *cfml::cfvariant_copy_value(const cfvariant *a) {
    auto *v = new cfvariant(*a);
    return tempRet(v);
}

 cfvariant *cfml::cfvariant_index(cfvariant *arr, const cfvariant *idx) {
    if (arr->m_type == cfvariant::Array) {
        int i = getIntValue(*idx);
        // A live query-column reference (q.a / q["a"] materialization) reads
        // through to the owning query's cells: reads reflect later query
        // mutations, and a row past the last one reads as empty instead of
        // throwing (CF: x = q["a"]; q["a"][2] = "YY" shows YY through x[2];
        // x[4] on a 3-row query is "").
        if (arr->m_queryColOwner && arr->m_queryColIndex >= 0) {
            QueryData *qd = arr->m_queryColOwner;
            int colIdx = arr->m_queryColIndex;
            if (colIdx >= 0 && colIdx < (int)qd->columns.size()) {
                QueryColumn &col = qd->columns[colIdx];
                if (i >= 1 && i <= (int)col.values.size()) {
                    return &col.values[i - 1];
                }
                auto *v = new cfvariant(cfvariant::Null);
                return tempRet(v);
            }
        }
        if (i < 1 || i > (int)arr->m_array->size()) {
            cf_throw_array_oob(i, 1, nullptr);
        }
        return &arr->m_array->at(i - 1);
    } else if (arr->m_type == cfvariant::String) {
        // CF indexes strings by position (returns the character). A re-copied
        // column reference degrades to such a scalar (its first cell), so
        // y[1] is the first character; an out-of-range position throws CF's
        // "Cannot access array element at position N." error.
        int i = getIntValue(*idx);
        if (i < 1 || i > (int)arr->m_str->length()) {
            throw webstrada::exception(string("Cannot access array element at position ") + string::number(i) + ".");
        }
        auto *v = new cfvariant(cfvariant::String);
        v->m_str->clear();
        v->m_str->append(arr->m_str->at(i - 1));
        return tempRet(v);
    } else if (arr->m_type == cfvariant::Xml) {
        string key = const_cast<cfvariant*>(idx)->toString();
        if (arr->m_struct) {
            auto it = arr->m_struct->find(key);
            if (it != arr->m_struct->end()) {
                return &it->second;
            }
            auto itChildren = arr->m_struct->find("XMLCHILDREN");
            if (itChildren != arr->m_struct->end() && itChildren->second.m_type == cfvariant::Array && itChildren->second.m_array) {
                std::vector<cfvariant> matches;
                for (auto &child : *itChildren->second.m_array) {
                    if (child.m_type == cfvariant::Xml && child.m_struct) {
                        auto itName = child.m_struct->find("XMLNAME");
                        if (itName != child.m_struct->end() && itName->second.toString().compareCaseInsensitive(key) == 0) {
                            matches.push_back(child);
                        }
                    }
                }
                if (matches.size() == 1) {
                    (*arr->m_struct)[key] = matches[0];
                    return &(*arr->m_struct)[key];
                } else if (matches.size() > 1) {
                    cfvariant childGroup(arr->m_upcase, false);
                    childGroup.set_type(cfvariant::Array);
                    childGroup.m_isXmlNodeList = true;
                    for (auto const& ch : matches) childGroup.insert(ch);
                    (*arr->m_struct)[key] = childGroup;
                    return &(*arr->m_struct)[key];
                }
            }
        }
        return &arr->set(key);
    } else if (arr->m_type == cfvariant::Struct) {
        string key = const_cast<cfvariant*>(idx)->toString();
        return &arr->set(key);
    } else if (arr->m_type == cfvariant::Component) {
        // o["member"]: this-scope data member first, then a method handle.
        string key = const_cast<cfvariant*>(idx)->toString();
        if (arr->m_struct) {
            auto it = arr->m_struct->find(key);
            if (it != arr->m_struct->end()) return &it->second;
        }
        if (cfvariant *h = componentMemberAccess(arr, key)) return h;
        string up = key;
        up.toUpper();
        throw webstrada::exception(string("Element ") + up + " is undefined in O.");
    } else if (arr->m_type == cfvariant::Query) {
        string key = const_cast<cfvariant*>(idx)->toString();
        // Column access takes precedence over pseudo-properties for bracket
        // access (verified against CF 2021: q["recordcount"][1] returns the
        // column data even when a "recordcount" column exists).
        int colIdx = arr->m_query->findColumn(key);
        if (colIdx >= 0) {
            QueryData *qd = arr->m_query;
            auto *colArr = new cfvariant(cfvariant::Array);
            for (auto &v : qd->columns[colIdx].values) colArr->insert(v);
            // Carry a back-reference so q["a"][1] = "z" writes through to the
            // query cell via cfvariant_index_assign. Bracket access produces a
            // column whose copies stay writable (CF: x = q["a"]; x[1] = "MUT"
            // writes through to the query). The reference is retained so the
            // query stays alive while any column copy exists.
            colArr->m_queryColOwner = query_data_retain(qd);
            colArr->m_queryColIndex = colIdx;
            colArr->m_queryColFromBracket = true;
            colArr->m_queryColWritable = true;
            return tempRet(colArr);
        }
        string upper = key;
        upper.toUpper();
        if (upper.equals("COLUMNLIST")) {
            auto *ret = new cfvariant(queryColumnList(arr));
            return tempRet(ret);
        }
        if (upper.equals("RECORDCOUNT")) {
            auto *ret = new cfvariant(queryRecordCount(arr));
            return tempRet(ret);
        }
        if (upper.equals("CURRENTROW")) {
            auto *ret = new cfvariant(arr->m_query->currentRow);
            return tempRet(ret);
        }
        throw webstrada::exception(string("Element ") + upper + " is undefined in " +
            (arr->m_query->currentRow > 0 ? "Q" : "Q") + ".");
    } else {
        // Indexing a scalar (e.g. y[1] where y holds the first-cell copy of an
        // integer column) throws CF's dereference error.
        throw webstrada::exception("You have attempted to dereference a scalar variable of type class " +
            scalarJavaTypeName(arr) + " as a structure with members.");
    }
}

cfvariant *cfml::cfvariant_index_named(cfvariant *arr, const cfvariant *idx, const char *varName, int dimension) {
    // Same as cfvariant_index but reports CF's named-variable out-of-bounds
    // message when the base is a simple variable (was BUGS.md "Array index
    // out-of-bounds error message").
    if (arr->m_type == cfvariant::Array) {
        int i = getIntValue(*idx);
        if (arr->m_queryColOwner && arr->m_queryColIndex >= 0) {
            QueryData *qd = arr->m_queryColOwner;
            int colIdx = arr->m_queryColIndex;
            if (colIdx >= 0 && colIdx < (int)qd->columns.size()) {
                QueryColumn &col = qd->columns[colIdx];
                if (i >= 1 && i <= (int)col.values.size()) {
                    return &col.values[i - 1];
                }
                auto *v = new cfvariant(cfvariant::Null);
                return tempRet(v);
            }
        }
        if (i < 1 || i > (int)arr->m_array->size()) {
            cf_throw_array_oob(i, dimension, varName);
        }
        return &arr->m_array->at(i - 1);
    }
    return cfml::cfvariant_index(arr, idx);
}

cfvariant *cfml::cfvariant_index_assign(cfvariant *arr, const cfvariant *idx, const cfvariant *val) {
    if (!arr) {
        throw webstrada::exception("Target of index assignment is not defined");
    }
    if (arr->m_type == cfvariant::Array) {
        int i = getIntValue(*idx);
        if (i < 1) {
            cf_throw_array_oob(i, 1, nullptr);
        }
        // A query-column reference writes through to the owning query's cell
        // for in-range rows. Writes past the last row are invisible (CF 2021:
        // x = q["a"]; x[4] = "w" on a 3-row query leaves x[4] and q.a[4] empty
        // and recordcount unchanged). A non-writable reference (dot-origin
        // copy) throws CF's dereference Expression error.
        if (arr->m_queryColOwner && arr->m_queryColIndex >= 0) {
            if (!arr->m_queryColWritable) {
                throw webstrada::exception("You have attempted to dereference a scalar variable of type class " +
                    queryColJavaTypeName(arr->m_queryColOwner, arr->m_queryColIndex, i - 1) +
                    " as a structure with members.");
            }
            QueryData *qd = arr->m_queryColOwner;
            int colIdx = arr->m_queryColIndex;
            if (colIdx >= 0 && colIdx < (int)qd->columns.size()) {
                QueryColumn &col = qd->columns[colIdx];
                if (i >= 1 && i <= (int)col.values.size()) {
                    col.values[i - 1] = coerceQueryCell(col.type, *val);
                    // Keep the materialized snapshot in sync (used by
                    // iteration / ArrayLen on the temp / ValueList).
                    if (i <= (int)arr->m_array->size()) {
                        arr->m_array->at(i - 1) = col.values[i - 1];
                    }
                    return &col.values[i - 1];
                }
            }
            // Phantom write (row past the last one): no visible effect.
            auto *v = new cfvariant(*val);
            return tempRet(v);
        }
        if (i > (int)arr->m_array->size()) {
            arr->m_array->resize(i, cfvariant(cfvariant::Null));
        }
        arr->m_array->at(i - 1) = *val;
        return &arr->m_array->at(i - 1);
    } else if (arr->m_type == cfvariant::Struct || arr->m_type == cfvariant::Xml) {
        string key = const_cast<cfvariant*>(idx)->toString();
        cfvariant &slot = arr->set(key);
        slot = *val;
        storeQueryColumnRef(slot);
        return &slot;
    } else if (arr->m_type == cfvariant::Component) {
        // o["x"] = v writes the component's this scope (keys uppercased, like
        // CF's this scope).
        string key = const_cast<cfvariant*>(idx)->toString();
        key.toUpper();
        if (!arr->m_struct) throw webstrada::exception("Component has no this scope");
        auto it = arr->m_struct->find(key);
        if (it == arr->m_struct->end()) {
            if (cf_component_has_method(arr, key.constData())) {
                cf_component_throw_method_not_found(arr, key.constData());
            }
        }
        cfvariant &slot = arr->set(key);
        slot = *val;
        storeQueryColumnRef(slot);
        return &slot;
    } else if (arr->m_type == cfvariant::Query) {
        // Bracket whole-column assignment q["a"] = v is rejected by CF
        // (verified on the RDS host: "An error occurred while trying to modify
        // the query named class coldfusion.sql.QueryTable."). Only the dot form
        // q.a = v writes the current row's cell (see assignQueryColumn).
        throw webstrada::exception("An error occurred while trying to modify the query named class coldfusion.sql.QueryTable.");
    } else {
        // Index assignment on a scalar throws CF's dereference error.
        throw webstrada::exception("You have attempted to dereference a scalar variable of type class " +
            scalarJavaTypeName(arr) + " as a structure with members.");
    }
}

cfvariant *cfml::cfvariant_index_assign_deep(cfvariant *root, const cfvariant **idxChain, int n, const cfvariant *val) {
    if (!root || n <= 0) {
        throw webstrada::exception("Target of index assignment is not defined");
    }
    cfvariant *cur = root;
    for (int k = 0; k < n - 1; k++) {
        if (cur->m_type == cfvariant::Array && !cur->m_queryColOwner) {
            // Plain-array intermediate: auto-create missing rows like CF
            // (a = ArrayNew(2); a[1][5] = 1 → a[1] becomes a nested array).
            int i = getIntValue(*idxChain[k]);
            if (i < 1) {
                cf_throw_array_oob(i, static_cast<int>(k + 1), nullptr);
            }
            if (i > (int)cur->m_array->size()) {
                cur->m_array->resize(i, cfvariant(cfvariant::Null));
            }
            cfvariant &slot = cur->m_array->at(i - 1);
            if (slot.m_type != cfvariant::Array) {
                slot = cfvariant(cfvariant::Array);
            }
            cur = &slot;
        } else {
            // Query / query-column / struct intermediate: the next level is
            // handled by the normal index paths (column-ref write-through,
            // struct member, ...).
            cur = cfvariant_index(cur, idxChain[k]);
        }
    }
    return cfvariant_index_assign(cur, idxChain[n - 1], val);
}

cfvariant *cfml::cfvariant_call_function(
    string &out,
    void *cgi, void *server, void *cookie, void *application, void *session, void *url, void *form, void *variables,
    const char *name, const cfvariant **args, int arg_count)
{
    // User-defined functions / closures are resolved as variables at runtime
    // (CF stores them in the variables scope). A variable holding a callable
    // Function value wins over built-in dispatch; a non-function variable
    // cannot be called. An undefined name falls through to the built-in
    // dispatch below and, if absent, throws "Variable NAME is undefined."
    cfvariant *udfVal = lookupVarWritable(name, cgi, server, cookie, application, session, url, form, variables);
    if (udfVal) {
        if (udfVal->m_type == cfvariant::Function && udfVal->m_udf && udfVal->m_udf->fn) {
            if (udfVal->m_udf->componentMethodIndex >= 0 && udfVal->m_udf->component) {
                return cf_component_method_handle_invoke(udfVal, args, arg_count, out, cgi, server, cookie,
                                                         application, session, url, form);
            }
            return cf_udf_invoke(udfVal, args, arg_count, out, cgi, server, cookie, application, session, url, form, variables);
        }
        if (udfVal->m_type != cfvariant::Function) {
            // CF: a built-in function wins over a non-function variable with
            // the same name (variables.dateformat = "x"; dateformat() still
            // calls the builtin — verified on CF 2025). Only a non-function
            // variable whose name is NOT a builtin raises "Entity has incorrect
            // type for being called as a function."
            string up(name);
            up.toUpper();
            if (!isKnownFunctionName(up)) {
                throw webstrada::exception("Entity has incorrect type for being called as a function.");
            }
        }
        // A plain built-in method handle stored in a variable: fall through to
        // built-in dispatch below.
    }

    // Inside a component method, an unqualified name that is not a variable may
    // be a component method (including private ones, which are callable from
    // within the component). CF resolves this before the built-ins.
    if (!g_udfCtx.empty()) {
        UdfCallCtx &inner = g_udfCtx.back();
        if (inner.component && inner.component->info) {
            std::string up(name);
            for (auto &c : up) c = (char)toupper((unsigned char)c);
            if (cf_component_has_method_on(inner.component, up.c_str())) {
                return cf_component_invoke_instance(inner.component, up.c_str(), args, arg_count,
                                                    out, cgi, server, cookie, application, session, url, form);
            }
        }
    }

    // Built-in dispatch: every path in cf_call_builtin_dispatch returns a
    // fresh heap allocation, so register it with the request's temp-variant
    // cleanup. The UDF/component returns above are already owned by their
    // callee and must NOT be registered here.
    string fname(name);
    fname.toUpper();
    return tempRet(cf_call_builtin_dispatch(out, cgi, server, cookie, application, session, url, form, variables,
                                            name, fname, args, arg_count));
}

// Built-in function dispatch (single caller: cfvariant_call_function). Every
// return in this function is a fresh heap allocation owned by the caller via
// tempRet.
static cfvariant *cf_call_builtin_dispatch(
    string &out,
    void *cgi, void *server, void *cookie, void *application, void *session, void *url, void *form, void *variables,
    const char *name, const string &fname, const cfvariant **args, int arg_count)
{
    if (fname.equals("CREATETIMESPAN")) {
        if (arg_count != 4) throw webstrada::exception("CreateTimeSpan requires exactly 4 arguments");
        return cf_createtimespan(args[0], args[1], args[2], args[3]);
    }
    if (fname.equals("DATEADD")) {
        if (arg_count != 3) throw webstrada::exception("DateAdd requires exactly 3 arguments");
        return cf_dateadd(args[0], args[1], args[2]);
    }
    if (fname.equals("DATECOMPARE")) {
        if (arg_count < 2 || arg_count > 3) throw webstrada::exception("DateCompare requires 2 or 3 arguments");
        return cf_datecompare(args[0], args[1], (arg_count == 3) ? args[2] : nullptr);
    }
    if (fname.equals("DATECONVERT")) {
        if (arg_count != 2) throw webstrada::exception("DateConvert requires exactly 2 arguments");
        return cf_dateconvert(args[0], args[1]);
    }
    if (fname.equals("DATEDIFF")) {
        if (arg_count != 3) throw webstrada::exception("DateDiff requires exactly 3 arguments");
        return cf_datediff(args[0], args[1], args[2]);
    }
    if (fname.equals("DATEPART")) {
        if (arg_count != 2) throw webstrada::exception("DatePart requires exactly 2 arguments");
        return cf_datepart(args[0], args[1]);
    }
    if (fname.equals("DAYOFWEEK")) {
        if (arg_count != 1) throw webstrada::exception("DayOfWeek requires exactly 1 argument");
        return cf_dayofweek(args[0]);
    }
    if (fname.equals("DAYOFWEEKASSTRING")) {
        if (arg_count < 1 || arg_count > 2) throw webstrada::exception("DayOfWeekAsString requires 1 or 2 arguments");
        return cf_dayofweekasstring(args[0], (arg_count == 2) ? args[1] : nullptr);
    }
    if (fname.equals("DAYOFYEAR")) {
        if (arg_count != 1) throw webstrada::exception("DayOfYear requires exactly 1 argument");
        return cf_dayofyear(args[0]);
    }
    if (fname.equals("DAYSINMONTH")) {
        if (arg_count != 1) throw webstrada::exception("DaysInMonth requires exactly 1 argument");
        return cf_daysinmonth(args[0]);
    }
    if (fname.equals("DAYSINYEAR")) {
        if (arg_count != 1) throw webstrada::exception("DaysInYear requires exactly 1 argument");
        return cf_daysinyear(args[0]);
    }
    if (fname.equals("FIRSTDAYOFMONTH")) {
        if (arg_count != 1) throw webstrada::exception("FirstDayOfMonth requires exactly 1 argument");
        return cf_firstdayofmonth(args[0]);
    }
    if (fname.equals("GETHTTPTIMESTRING")) {
        if (arg_count > 1) throw webstrada::exception("GetHttpTimeString requires 0 or 1 arguments");
        return cf_gethttptimestring((arg_count == 1) ? args[0] : nullptr);
    }
    if (fname.equals("GETHTTPREQUESTDATA")) {
        if (arg_count > 1) throw webstrada::exception("GetHttpRequestData requires 0 or 1 arguments");
        return cf_gethttprequestdata(cgi, (arg_count == 1) ? args[0] : nullptr);
    }
    if (fname.equals("GETTIMEZONEINFO")) {
        if (arg_count != 0) throw webstrada::exception("GetTimeZoneInfo requires exactly 0 arguments");
        return cf_gettimezoneinfo();
    }
    if (fname.equals("ISBINARY")) {
        if (arg_count != 1) throw webstrada::exception("IsBinary requires exactly 1 argument");
        return cf_isbinary(args[0]);
    }
    if (fname.equals("ISBOOLEAN")) {
        if (arg_count != 1) throw webstrada::exception("IsBoolean requires exactly 1 argument");
        return cf_isboolean(args[0]);
    }
    if (fname.equals("ISCLOSURE")) {
        if (arg_count != 1) throw webstrada::exception("IsClosure requires exactly 1 argument");
        return cf_isclosure(args[0]);
    }
    if (fname.equals("ISCUSTOMFUNCTION")) {
        if (arg_count != 1) throw webstrada::exception("IsCustomFunction requires exactly 1 argument");
        return cf_iscustomfunction(args[0]);
    }
    if (fname.equals("ISDEFINED")) {
        if (arg_count != 1) throw webstrada::exception("IsDefined requires exactly 1 argument");
        return cf_isdefined(args[0], cgi, server, cookie, application, session, url, form, variables);
    }
    if (fname.equals("ISFILEOBJECT")) {
        if (arg_count != 1) throw webstrada::exception("IsFileObject requires exactly 1 argument");
        return cf_isfileobject(args[0]);
    }
    if (fname.equals("ISIMAGE")) {
        if (arg_count != 1) throw webstrada::exception("IsImage requires exactly 1 argument");
        return cf_isimage(args[0]);
    }
    if (fname.equals("ISNULL")) {
        if (arg_count != 1) throw webstrada::exception("IsNull requires exactly 1 argument");
        return cf_isnull(args[0]);
    }
    if (fname.equals("ISNUMERIC")) {
        if (arg_count != 1) throw webstrada::exception("IsNumeric requires exactly 1 argument");
        return cf_isnumeric(args[0]);
    }
    if (fname.equals("LSISNUMERIC")) {
        if (arg_count != 1) throw webstrada::exception("LSIsNumeric requires exactly 1 argument");
        return cf_lsisnumeric(args[0]);
    }
    if (fname.equals("ISOBJECT")) {
        if (arg_count != 1) throw webstrada::exception("IsObject requires exactly 1 argument");
        return cf_isobject(args[0]);
    }
    if (fname.equals("ISSIMPLEVALUE")) {
        if (arg_count != 1) throw webstrada::exception("IsSimpleValue requires exactly 1 argument");
        return cf_issimplevalue(args[0]);
    }
    if (fname.equals("ISSTRUCT")) {
        if (arg_count != 1) throw webstrada::exception("IsStruct requires exactly 1 argument");
        return cf_isstruct(args[0]);
    }
    if (fname.equals("FILEISEOF")) {
        if (arg_count != 1) throw webstrada::exception("FileIsEOF requires exactly 1 argument");
        return cf_fileiseof(args[0]);
    }
    if (fname.equals("ISDATEOBJECT")) {
        if (arg_count != 1) throw webstrada::exception("IsDateObject requires exactly 1 argument");
        return cf_isdateobject(args[0]);
    }
    if (fname.equals("ISLEAPYEAR")) {
        if (arg_count != 1) throw webstrada::exception("IsLeapYear requires exactly 1 argument");
        return cf_isleapyear(args[0]);
    }
    if (fname.equals("ISNUMERICDATE")) {
        if (arg_count != 1) throw webstrada::exception("IsNumericDate requires exactly 1 argument");
        return cf_isnumericdate(args[0]);
    }
    if (fname.equals("LSDATEFORMAT")) {
        if (arg_count < 1 || arg_count > 3) throw webstrada::exception("LSDateFormat requires between 1 and 3 arguments");
        return cf_lsdateformat(args[0], (arg_count >= 2) ? args[1] : nullptr, (arg_count == 3) ? args[2] : nullptr);
    }
    if (fname.equals("LSDATETIMEFORMAT")) {
        if (arg_count < 1 || arg_count > 3) throw webstrada::exception("LSDateTimeFormat requires between 1 and 3 arguments");
        return cf_lsdatetimeformat(args[0], (arg_count >= 2) ? args[1] : nullptr, (arg_count == 3) ? args[2] : nullptr);
    }
    if (fname.equals("LSISDATE")) {
        if (arg_count < 1 || arg_count > 2) throw webstrada::exception("LSIsDate requires 1 or 2 arguments");
        return cf_lsisdate(args[0], (arg_count == 2) ? args[1] : nullptr);
    }
    if (fname.equals("LSCURRENCYFORMAT")) {
        if (arg_count < 1 || arg_count > 3) throw webstrada::exception("LSCurrencyFormat requires between 1 and 3 arguments");
        return cf_lscurrencyformat(args[0], (arg_count >= 2) ? args[1] : nullptr, (arg_count == 3) ? args[2] : nullptr);
    }
    if (fname.equals("LSEUROCURRENCYFORMAT")) {
        if (arg_count < 1 || arg_count > 3) throw webstrada::exception("LSEuroCurrencyFormat requires between 1 and 3 arguments");
        return cf_lseurocurrencyformat(args[0], (arg_count >= 2) ? args[1] : nullptr, (arg_count == 3) ? args[2] : nullptr);
    }
    if (fname.equals("LSISCURRENCY")) {
        if (arg_count < 1 || arg_count > 2) throw webstrada::exception("LSIsCurrency requires 1 or 2 arguments");
        return cf_lsiscurrency(args[0], (arg_count == 2) ? args[1] : nullptr);
    }
    if (fname.equals("LSNUMBERFORMAT")) {
        if (arg_count < 1 || arg_count > 3) throw webstrada::exception("LSNumberFormat requires between 1 and 3 arguments");
        return cf_lsnumberformat(args[0], (arg_count >= 2) ? args[1] : nullptr, (arg_count == 3) ? args[2] : nullptr);
    }
    if (fname.equals("LSPARSECURRENCY")) {
        if (arg_count < 1 || arg_count > 2) throw webstrada::exception("LSParseCurrency requires 1 or 2 arguments");
        return cf_lsparsecurrency(args[0], (arg_count == 2) ? args[1] : nullptr);
    }
    if (fname.equals("LSPARSEEUROCURRENCY")) {
        if (arg_count < 1 || arg_count > 2) throw webstrada::exception("LSParseEuroCurrency requires 1 or 2 arguments");
        return cf_lsparseeurocurrency(args[0], (arg_count == 2) ? args[1] : nullptr);
    }
    if (fname.equals("LSPARSENUMBER")) {
        if (arg_count < 1 || arg_count > 2) throw webstrada::exception("LSParseNumber requires 1 or 2 arguments");
        return cf_lsparsenumber(args[0], (arg_count == 2) ? args[1] : nullptr);
    }
    if (fname.equals("GETLOCALE")) {
        if (arg_count != 0) throw webstrada::exception("GetLocale requires 0 arguments");
        return cf_getlocale();
    }
    if (fname.equals("SETLOCALE")) {
        if (arg_count != 1) throw webstrada::exception("SetLocale requires exactly 1 argument");
        return cf_setlocale(args[0]);
    }
    if (fname.equals("LSPARSEDATETIME")) {
        if (arg_count < 1 || arg_count > 2) throw webstrada::exception("LSParseDateTime requires 1 or 2 arguments");
        return cf_lsparsedatetime(args[0], (arg_count == 2) ? args[1] : nullptr);
    }
    if (fname.equals("LSTIMEFORMAT")) {
        if (arg_count < 1 || arg_count > 3) throw webstrada::exception("LSTimeFormat requires between 1 and 3 arguments");
        return cf_lstimeformat(args[0], (arg_count >= 2) ? args[1] : nullptr, (arg_count == 3) ? args[2] : nullptr);
    }
    if (fname.equals("MONTHASSTRING")) {
        if (arg_count < 1 || arg_count > 2) throw webstrada::exception("MonthAsString requires 1 or 2 arguments");
        return cf_monthasstring(args[0], (arg_count == 2) ? args[1] : nullptr);
    }
    if (fname.equals("PARSEDATETIME")) {
        if (arg_count < 1 || arg_count > 2) throw webstrada::exception("ParseDateTime requires 1 or 2 arguments");
        return cf_parsedatetime(args[0], (arg_count == 2) ? args[1] : nullptr);
    }
    if (fname.equals("QUARTER")) {
        if (arg_count != 1) throw webstrada::exception("Quarter requires exactly 1 argument");
        return cf_quarter(args[0]);
    }
    if (fname.equals("SETDAY")) {
        if (arg_count != 2) throw webstrada::exception("SetDay requires exactly 2 arguments");
        return cf_setday(args[0], args[1]);
    }
    if (fname.equals("SETHOUR")) {
        if (arg_count != 2) throw webstrada::exception("SetHour requires exactly 2 arguments");
        return cf_sethour(args[0], args[1]);
    }
    if (fname.equals("SETMINUTE")) {
        if (arg_count != 2) throw webstrada::exception("SetMinute requires exactly 2 arguments");
        return cf_setminute(args[0], args[1]);
    }
    if (fname.equals("SETMONTH")) {
        if (arg_count != 2) throw webstrada::exception("SetMonth requires exactly 2 arguments");
        return cf_setmonth(args[0], args[1]);
    }
    if (fname.equals("SETSECOND")) {
        if (arg_count != 2) throw webstrada::exception("SetSecond requires exactly 2 arguments");
        return cf_setsecond(args[0], args[1]);
    }
    if (fname.equals("SETYEAR")) {
        if (arg_count != 2) throw webstrada::exception("SetYear requires exactly 2 arguments");
        return cf_setyear(args[0], args[1]);
    }
    if (fname.equals("WEEK")) {
        if (arg_count != 1) throw webstrada::exception("Week requires exactly 1 argument");
        return cf_week(args[0]);
    }

    // Handle mutating functions directly
    cfvariant *mut_arg0 = (arg_count > 0) ? const_cast<cfvariant*>(args[0]) : nullptr;

    if (fname.equals("QUERYADDROW")) {
        if (arg_count < 1 || arg_count > 2) throw webstrada::exception("QueryAddRow requires 1 or 2 arguments");
        if (!mut_arg0 || mut_arg0->m_type != cfvariant::Query) throw webstrada::exception("QueryAddRow: First argument must be a query");
        return cf_queryaddrow(mut_arg0, (arg_count == 2) ? args[1] : nullptr);
    }
    if (fname.equals("QUERYSETCELL")) {
        if (arg_count < 3 || arg_count > 4) throw webstrada::exception("QuerySetCell requires 3 or 4 arguments");
        if (!mut_arg0 || mut_arg0->m_type != cfvariant::Query) throw webstrada::exception("QuerySetCell: First argument must be a query");
        return cf_querysetcell(mut_arg0, args[1], args[2], (arg_count == 4) ? args[3] : nullptr);
    }
    if (fname.equals("QUERYADDCOLUMN")) {
        if (arg_count < 3 || arg_count > 4) throw webstrada::exception("QueryAddColumn requires 3 or 4 arguments");
        if (!mut_arg0 || mut_arg0->m_type != cfvariant::Query) throw webstrada::exception("QueryAddColumn: First argument must be a query");
        if (arg_count == 3) return cf_queryaddcolumn(mut_arg0, args[1], args[2], nullptr);
        return cf_queryaddcolumn(mut_arg0, args[1], args[2], args[3]);
    }
    if (fname.equals("QUERYGETROW")) {
        if (arg_count != 2) throw webstrada::exception("QueryGetRow requires exactly 2 arguments");
        return cf_querygetrow(args[0], args[1]);
    }
    if (fname.equals("QUERYKEYEXISTS")) {
        if (arg_count != 2) throw webstrada::exception("QueryKeyExists requires exactly 2 arguments");
        return cf_querykeyexists(args[0], args[1]);
    }
    if (fname.equals("QUERYCONVERTFORGRID")) {
        if (arg_count != 3) throw webstrada::exception("QueryConvertForGrid requires exactly 3 arguments");
        return cf_queryconvertforgrid(args[0], args[1], args[2]);
    }
    if (fname.equals("QUERYEXECUTE")) {
        if (arg_count < 1 || arg_count > 3) throw webstrada::exception("QueryExecute requires 1 to 3 arguments");
        return cf_queryexecute(args[0], arg_count >= 2 ? args[1] : nullptr, arg_count == 3 ? args[2] : nullptr,
                               cgi, server, cookie, application, session, url, form, variables);
    }
    if (fname.equals("QUERYEACH")) {
        if (arg_count < 2 || arg_count > 4) throw webstrada::exception("QueryEach requires 2 to 4 arguments");
        return cf_queryeach(args[0], args[1], arg_count >= 3 ? args[2] : nullptr, arg_count == 4 ? args[3] : nullptr,
                            out, cgi, server, cookie, application, session, url, form, variables);
    }
    if (fname.equals("QUERYFILTER")) {
        if (arg_count < 2 || arg_count > 4) throw webstrada::exception("QueryFilter requires 2 to 4 arguments");
        return cf_queryfilter(args[0], args[1], arg_count >= 3 ? args[2] : nullptr, arg_count == 4 ? args[3] : nullptr,
                              out, cgi, server, cookie, application, session, url, form, variables);
    }
    if (fname.equals("QUERYGETRESULT")) {
        if (arg_count != 1) throw webstrada::exception("QueryGetResult requires exactly 1 argument");
        return cf_querygetresult(args[0]);
    }
    if (fname.equals("QUERYMAP")) {
        if (arg_count < 2 || arg_count > 4) throw webstrada::exception("QueryMap requires 2 to 4 arguments");
        return cf_querymap(args[0], args[1], arg_count >= 3 ? args[2] : nullptr, arg_count == 4 ? args[3] : nullptr,
                           out, cgi, server, cookie, application, session, url, form, variables);
    }
    if (fname.equals("QUERYREDUCE")) {
        if (arg_count < 2 || arg_count > 3) throw webstrada::exception("QueryReduce requires 2 or 3 arguments");
        return cf_queryreduce(args[0], args[1], arg_count == 3 ? args[2] : nullptr,
                              out, cgi, server, cookie, application, session, url, form, variables);
    }
    if (fname.equals("QUOTEDVALUELIST")) {
        if (arg_count < 1 || arg_count > 2) throw webstrada::exception("QuotedValueList requires 1 or 2 arguments");
        return cf_quotedvaluelist(args[0], arg_count == 2 ? args[1] : nullptr);
    }
    if (fname.equals("VALUELIST")) {
        if (arg_count < 1 || arg_count > 2) throw webstrada::exception("ValueList requires 1 or 2 arguments");
        return cf_valuelist(args[0], (arg_count == 2) ? args[1] : nullptr);
    }

    if (fname.equals("ARRAYEACH")) {
        if (arg_count < 2 || arg_count > 4) throw webstrada::exception("ArrayEach requires 2 to 4 arguments");
        return cf_arrayeach(args[0], args[1], arg_count >= 3 ? args[2] : nullptr, arg_count == 4 ? args[3] : nullptr,
                            out, cgi, server, cookie, application, session, url, form, variables);
    }
    if (fname.equals("ARRAYFILTER")) {
        if (arg_count < 2 || arg_count > 4) throw webstrada::exception("ArrayFilter requires 2 to 4 arguments");
        return cf_arrayfilter(args[0], args[1], arg_count >= 3 ? args[2] : nullptr, arg_count == 4 ? args[3] : nullptr,
                              out, cgi, server, cookie, application, session, url, form, variables);
    }
    if (fname.equals("ARRAYREDUCE")) {
        if (arg_count < 2 || arg_count > 3) throw webstrada::exception("ArrayReduce requires 2 or 3 arguments");
        return cf_arrayreduce(args[0], args[1], arg_count == 3 ? args[2] : nullptr,
                              out, cgi, server, cookie, application, session, url, form, variables);
    }
    if (fname.equals("LISTEACH")) {
        if (arg_count < 2 || arg_count > 4) throw webstrada::exception("ListEach requires 2 to 4 arguments");
        return cf_listeach(args[0], args[1], arg_count >= 3 ? args[2] : nullptr, arg_count == 4 ? args[3] : nullptr,
                           out, cgi, server, cookie, application, session, url, form, variables);
    }
    if (fname.equals("LISTFILTER")) {
        if (arg_count < 2 || arg_count > 4) throw webstrada::exception("ListFilter requires 2 to 4 arguments");
        return cf_listfilter(args[0], args[1], arg_count >= 3 ? args[2] : nullptr, arg_count == 4 ? args[3] : nullptr,
                             out, cgi, server, cookie, application, session, url, form, variables);
    }
    if (fname.equals("LISTGETDUPLICATES")) {
        if (arg_count < 1 || arg_count > 2) throw webstrada::exception("ListGetDuplicates requires 1 or 2 arguments");
        return cf_listgetduplicates(args[0], arg_count == 2 ? args[1] : nullptr);
    }
    if (fname.equals("LISTMAP")) {
        if (arg_count < 2 || arg_count > 4) throw webstrada::exception("ListMap requires 2 to 4 arguments");
        return cf_listmap(args[0], args[1], arg_count >= 3 ? args[2] : nullptr, arg_count == 4 ? args[3] : nullptr,
                          out, cgi, server, cookie, application, session, url, form, variables);
    }
    if (fname.equals("LISTQUALIFY")) {
        if (arg_count < 2 || arg_count > 5) throw webstrada::exception("ListQualify requires 2 to 5 arguments");
        return cf_listqualify(args[0], args[1], arg_count >= 3 ? args[2] : nullptr,
                              arg_count >= 4 ? args[3] : nullptr, arg_count == 5 ? args[4] : nullptr);
    }
    if (fname.equals("LISTREDUCE")) {
        if (arg_count < 2 || arg_count > 5) throw webstrada::exception("ListReduce requires 2 to 5 arguments");
        return cf_listreduce(args[0], args[1], arg_count >= 3 ? args[2] : nullptr,
                             arg_count >= 4 ? args[3] : nullptr, arg_count == 5 ? args[4] : nullptr,
                             out, cgi, server, cookie, application, session, url, form, variables);
    }
    if (fname.equals("LISTREMOVEDUPLICATES")) {
        if (arg_count < 1 || arg_count > 3) throw webstrada::exception("ListRemoveDuplicates requires 1 to 3 arguments");
        return cf_listremoveduplicates(args[0], arg_count >= 2 ? args[1] : nullptr, arg_count == 3 ? args[2] : nullptr);
    }
    if (fname.equals("LISTSORT")) {
        if (arg_count < 2 || arg_count > 6) throw webstrada::exception("ListSort requires 2 to 6 arguments");
        return cf_listsort(args[0], args[1], arg_count >= 3 ? args[2] : nullptr,
                           arg_count >= 4 ? args[3] : nullptr, arg_count >= 5 ? args[4] : nullptr,
                           arg_count == 6 ? args[5] : nullptr,
                           out, cgi, server, cookie, application, session, url, form, variables);
    }
    if (fname.equals("LISTVALUECOUNT")) {
        if (arg_count < 2 || arg_count > 3) throw webstrada::exception("ListValueCount requires 2 or 3 arguments");
        return cf_listvaluecount(args[0], args[1], arg_count == 3 ? args[2] : nullptr);
    }
    if (fname.equals("LISTVALUECOUNTNOCASE")) {
        if (arg_count < 2 || arg_count > 3) throw webstrada::exception("ListValueCountNoCase requires 2 or 3 arguments");
        return cf_listvaluecountnocase(args[0], args[1], arg_count == 3 ? args[2] : nullptr);
    }
    if (fname.equals("STRUCTAPPEND")) {
        if (arg_count < 2 || arg_count > 3) throw webstrada::exception("StructAppend requires 2 or 3 arguments");
        if (!mut_arg0 || mut_arg0->m_type != cfvariant::Struct) throw webstrada::exception("StructAppend: First argument must be a struct");
        return cf_structappend(mut_arg0, args[1], arg_count == 3 ? args[2] : nullptr);
    }
    if (fname.equals("STRUCTCOPY")) {
        if (arg_count != 1) throw webstrada::exception("StructCopy requires exactly 1 argument");
        return cf_structcopy(args[0]);
    }
    if (fname.equals("STRUCTEACH")) {
        if (arg_count < 2 || arg_count > 4) throw webstrada::exception("StructEach requires 2 to 4 arguments");
        return cf_structeach(args[0], args[1], arg_count >= 3 ? args[2] : nullptr, arg_count == 4 ? args[3] : nullptr,
                             out, cgi, server, cookie, application, session, url, form, variables);
    }
    if (fname.equals("STRUCTFILTER")) {
        if (arg_count < 2 || arg_count > 4) throw webstrada::exception("StructFilter requires 2 to 4 arguments");
        return cf_structfilter(args[0], args[1], arg_count >= 3 ? args[2] : nullptr, arg_count == 4 ? args[3] : nullptr,
                               out, cgi, server, cookie, application, session, url, form, variables);
    }
    if (fname.equals("STRUCTFINDKEY")) {
        if (arg_count < 2 || arg_count > 3) throw webstrada::exception("StructFindKey requires 2 or 3 arguments");
        return cf_structfindkey(args[0], args[1], arg_count == 3 ? args[2] : nullptr);
    }
    if (fname.equals("STRUCTFINDVALUE")) {
        if (arg_count < 2 || arg_count > 3) throw webstrada::exception("StructFindValue requires 2 or 3 arguments");
        return cf_structfindvalue(args[0], args[1], arg_count == 3 ? args[2] : nullptr);
    }
    if (fname.equals("STRUCTGET")) {
        if (arg_count != 1) throw webstrada::exception("StructGet requires exactly 1 argument");
        return cf_structget(args[0], variables);
    }
    if (fname.equals("STRUCTGETMETADATA")) {
        if (arg_count != 1) throw webstrada::exception("StructGetMetadata requires exactly 1 argument");
        return cf_structgetmetadata(args[0]);
    }
    if (fname.equals("STRUCTMAP")) {
        if (arg_count < 2 || arg_count > 4) throw webstrada::exception("StructMap requires 2 to 4 arguments");
        return cf_structmap(args[0], args[1], arg_count >= 3 ? args[2] : nullptr, arg_count == 4 ? args[3] : nullptr,
                            out, cgi, server, cookie, application, session, url, form, variables);
    }
    if (fname.equals("STRUCTREDUCE")) {
        if (arg_count < 2 || arg_count > 3) throw webstrada::exception("StructReduce requires 2 or 3 arguments");
        return cf_structreduce(args[0], args[1], arg_count == 3 ? args[2] : nullptr,
                               out, cgi, server, cookie, application, session, url, form, variables);
    }
    if (fname.equals("STRUCTSETMETADATA")) {
        if (arg_count != 2) throw webstrada::exception("StructSetMetadata requires exactly 2 arguments");
        if (!mut_arg0 || mut_arg0->m_type != cfvariant::Struct) throw webstrada::exception("StructSetMetadata: First argument must be a struct");
        return cf_structsetmetadata(mut_arg0, args[1]);
    }
    if (fname.equals("STRUCTSORT")) {
        if (arg_count < 2 || arg_count > 5) throw webstrada::exception("StructSort requires 2 to 5 arguments");
        return cf_structsort(args[0], args[1], arg_count >= 3 ? args[2] : nullptr,
                             arg_count >= 4 ? args[3] : nullptr, arg_count == 5 ? args[4] : nullptr,
                             out, cgi, server, cookie, application, session, url, form, variables);
    }
    if (fname.equals("STRUCTTOSORTED")) {
        if (arg_count < 2 || arg_count > 4) throw webstrada::exception("StructToSorted requires 2 to 4 arguments");
        return cf_structtosorted(args[0], args[1], arg_count >= 3 ? args[2] : nullptr,
                                 arg_count == 4 ? args[3] : nullptr,
                                 out, cgi, server, cookie, application, session, url, form, variables);
    }

    if (fname.equals("ARRAYAPPEND")) {
        if (arg_count != 2) throw webstrada::exception("ArrayAppend requires exactly 2 arguments");
        if (mut_arg0->m_isXmlNodeList) throwXmlNodeListUnsupported("ArrayAppend");
        if (!isCfArray(mut_arg0)) throwNotArrayError(mut_arg0);
        mut_arg0->insert(*args[1]);
        return cfvariant_create_bool(true);
    }
    if (fname.equals("ARRAYPREPEND")) {
        if (arg_count != 2) throw webstrada::exception("ArrayPrepend requires exactly 2 arguments");
        if (mut_arg0->m_isXmlNodeList) throwXmlNodeListUnsupported("ArrayPrepend");
        if (!isCfArray(mut_arg0)) throwNotArrayError(mut_arg0);
        mut_arg0->m_array->insert(mut_arg0->m_array->begin(), *args[1]);
        return cfvariant_create_bool(true);
    }
    if (fname.equals("ARRAYCLEAR")) {
        if (arg_count != 1) throw webstrada::exception("ArrayClear requires exactly 1 argument");
        if (mut_arg0->m_isXmlNodeList) throwXmlNodeListUnsupported("ArrayClear");
        if (!isCfArray(mut_arg0)) throwNotArrayError(mut_arg0);
        mut_arg0->m_array->clear();
        return cfvariant_create_bool(true);
    }
    if (fname.equals("ARRAYDELETEAT")) {
        if (arg_count != 2) throw webstrada::exception("ArrayDeleteAt requires exactly 2 arguments");
        if (mut_arg0->m_isXmlNodeList) throwXmlNodeListUnsupported("ArrayDeleteAt");
        if (!isCfArray(mut_arg0)) throwNotArrayError(mut_arg0);
        int idx = getIntValue(*args[1]);
        if (idx < 1 || idx > (int)mut_arg0->m_array->size()) throw webstrada::exception("ArrayDeleteAt: Index out of bounds");
        mut_arg0->m_array->erase(mut_arg0->m_array->begin() + (idx - 1));
        return cfvariant_create_bool(true);
    }
    if (fname.equals("ARRAYINSERTAT")) {
        if (arg_count != 3) throw webstrada::exception("ArrayInsertAt requires exactly 3 arguments");
        if (mut_arg0->m_isXmlNodeList) throwXmlNodeListUnsupported("ArrayInsertAt");
        if (!isCfArray(mut_arg0)) throwNotArrayError(mut_arg0);
        int idx = getIntValue(*args[1]);
        if (idx < 1 || idx > (int)mut_arg0->m_array->size() + 1) throw webstrada::exception("ArrayInsertAt: Index out of bounds");
        mut_arg0->m_array->insert(mut_arg0->m_array->begin() + (idx - 1), *args[2]);
        return cfvariant_create_bool(true);
    }
    if (fname.equals("ARRAYFIRST")) {
        if (arg_count != 1) throw webstrada::exception("ArrayFirst requires exactly 1 argument");
        if (mut_arg0->m_isXmlNodeList) throwXmlNodeListUnsupported("ArrayFirst");
        if (!isCfArray(mut_arg0)) throwNotArrayError(mut_arg0);
        if (mut_arg0->m_array->empty()) throw webstrada::exception("java.lang.RuntimeException", "Array is empty.Cannot return first element of array.", "");
        cfvariant *ret = new cfvariant(mut_arg0->m_array->front());
        return ret;
    }
    if (fname.equals("ARRAYLAST")) {
        if (arg_count != 1) throw webstrada::exception("ArrayLast requires exactly 1 argument");
        if (mut_arg0->m_isXmlNodeList) throwXmlNodeListUnsupported("ArrayLast");
        if (!isCfArray(mut_arg0)) throwNotArrayError(mut_arg0);
        if (mut_arg0->m_array->empty()) throw webstrada::exception("java.lang.RuntimeException", "Array is empty.Cannot return last element of array.", "");
        cfvariant *ret = new cfvariant(mut_arg0->m_array->back());
        return ret;
    }
    if (fname.equals("ARRAYPOP")) {
        if (arg_count != 1) throw webstrada::exception("ArrayPop requires exactly 1 argument");
        if (mut_arg0->m_isXmlNodeList) throwXmlNodeListUnsupported("ArrayPop");
        if (!isCfArray(mut_arg0)) throwNotArrayError(mut_arg0);
        if (mut_arg0->m_array->empty()) throw webstrada::exception("Empty Array.");
        cfvariant *ret = new cfvariant(mut_arg0->m_array->back());
        mut_arg0->m_array->pop_back();
        return ret;
    }
    if (fname.equals("ARRAYSHIFT")) {
        if (arg_count != 1) throw webstrada::exception("ArrayShift requires exactly 1 argument");
        if (mut_arg0->m_isXmlNodeList) throwXmlNodeListUnsupported("ArrayShift");
        if (!isCfArray(mut_arg0)) throwNotArrayError(mut_arg0);
        if (mut_arg0->m_array->empty()) throw webstrada::exception("Empty Array.");
        cfvariant *ret = new cfvariant(mut_arg0->m_array->front());
        mut_arg0->m_array->erase(mut_arg0->m_array->begin());
        return ret;
    }
    if (fname.equals("ARRAYRESIZE")) {
        if (arg_count != 2) throw webstrada::exception("ArrayResize requires exactly 2 arguments");
        if (mut_arg0->m_isXmlNodeList) throwXmlNodeListUnsupported("ArrayResize");
        if (!isCfArray(mut_arg0)) throwNotArrayError(mut_arg0);
        int sz = getIntValue(*args[1]);
        mut_arg0->m_array->resize(sz);
        return cfvariant_create_bool(true);
    }
    if (fname.equals("ARRAYSET")) {
        if (arg_count != 4) throw webstrada::exception("ArraySet requires exactly 4 arguments");
        if (mut_arg0->m_isXmlNodeList) throwXmlNodeListUnsupported("ArraySet");
        if (!isCfArray(mut_arg0)) throwNotArrayError(mut_arg0);
        int start = getIntValue(*args[1]);
        int end = getIntValue(*args[2]);
        for (int i = start; i <= end; i++) {
            if (i >= 1 && i <= (int)mut_arg0->m_array->size()) {
                mut_arg0->m_array->at(i - 1) = *args[3];
            }
        }
        return cfvariant_create_bool(true);
    }
    if (fname.equals("ARRAYDELETE")) {
        if (arg_count != 2) throw webstrada::exception("ArrayDelete requires exactly 2 arguments");
        if (mut_arg0->m_isXmlNodeList) throwXmlNodeListUnsupported("ArrayDelete");
        if (!isCfArray(mut_arg0)) throwNotArrayError(mut_arg0);
        cfvariant val = *args[1];
        bool deleted = false;
        auto &arr = *mut_arg0->m_array;
        for (auto it = arr.begin(); it != arr.end(); ++it) {
            if (cfvariantsEqual(*it, val)) {
                arr.erase(it);
                deleted = true;
                break;
            }
        }
        return cfvariant_create_bool(deleted);
    }
    if (fname.equals("ARRAYDELETENOCASE")) {
        if (arg_count != 2) throw webstrada::exception("ArrayDeleteNoCase requires exactly 2 arguments");
        if (mut_arg0->m_isXmlNodeList) throwXmlNodeListUnsupported("ArrayDeleteNoCase");
        if (!isCfArray(mut_arg0)) throwNotArrayError(mut_arg0);
        cfvariant val = *args[1];
        bool deleted = false;
        auto &arr = *mut_arg0->m_array;
        for (auto it = arr.begin(); it != arr.end(); ++it) {
            if (cfvariantsEqualNoCase(*it, val)) {
                arr.erase(it);
                deleted = true;
                break;
            }
        }
        return cfvariant_create_bool(deleted);
    }
    if (fname.equals("ARRAYSWAP")) {
        if (arg_count != 3) throw webstrada::exception("ArraySwap requires exactly 3 arguments");
        if (mut_arg0->m_isXmlNodeList) throwXmlNodeListUnsupported("ArraySwap");
        if (!isCfArray(mut_arg0)) throwNotArrayError(mut_arg0);
        int idx1 = getIntValue(*args[1]);
        int idx2 = getIntValue(*args[2]);
        if (idx1 < 1 || idx1 > (int)mut_arg0->m_array->size() || idx2 < 1 || idx2 > (int)mut_arg0->m_array->size()) {
            throw webstrada::exception("ArraySwap: Index out of bounds");
        }
        std::swap(mut_arg0->m_array->at(idx1 - 1), mut_arg0->m_array->at(idx2 - 1));
        return cfvariant_create_bool(true);
    }
    if (fname.equals("ARRAYSORT")) {
        if (arg_count < 2 || arg_count > 3) throw webstrada::exception("ArraySort requires 2 or 3 arguments");
        if (!isCfArray(mut_arg0)) throwNotArrayError(mut_arg0);
        cfvariant arg2 = *args[1];
        string arg2_str = arg2.toString();
        arg2_str.toUpper();

        bool isCallback = !(arg2_str.equals("NUMERIC") || arg2_str.equals("TEXT") || arg2_str.equals("TEXTNOCASE"));

        if (isCallback) {
            std::stable_sort(mut_arg0->m_array->begin(), mut_arg0->m_array->end(),
                [&](const cfvariant &a, const cfvariant &b) {
                    std::vector<cfvariant> cbArgs = { a, b };
                    cfvariant cbRes = callCallback(out, arg2, cbArgs, cgi, server, cookie, application, session, url, form, variables);
                    return getIntValue(cbRes) < 0;
                }
            );
        } else {
            string sortOrder = "ASC";
            if (arg_count == 3) {
                cfvariant arg3 = *args[2];
                sortOrder = arg3.toString();
                sortOrder.toUpper();
            }
            bool asc = !sortOrder.equals("DESC");

            std::stable_sort(mut_arg0->m_array->begin(), mut_arg0->m_array->end(),
                [&](const cfvariant &a, const cfvariant &b) {
                    bool less = false;
                    if (arg2_str.equals("NUMERIC")) {
                        try {
                            less = getDoubleValue(a) < getDoubleValue(b);
                        } catch (...) {
                            less = false;
                        }
                    } else if (arg2_str.equals("TEXT")) {
                        string sa = const_cast<cfvariant&>(a).toString();
                        string sb = const_cast<cfvariant&>(b).toString();
                        const char *s1 = sa.constData();
                        const char *s2 = sb.constData();
                        if (s1 && s2) less = strcmp(s1, s2) < 0;
                        else less = !s1;
                    } else { // TEXTNOCASE
                        string sa = const_cast<cfvariant&>(a).toString();
                        string sb = const_cast<cfvariant&>(b).toString();
                        const char *s1 = sa.constData();
                        const char *s2 = sb.constData();
                        if (s1 && s2) less = strcasecmp(s1, s2) < 0;
                        else less = !s1;
                    }
                    return asc ? less : !less;
                }
            );
        }
        return cfvariant_create_bool(true);
    }
    if (fname.equals("STRUCTINSERT")) {
        if (arg_count < 3 || arg_count > 4) throw webstrada::exception("StructInsert requires 3 or 4 arguments");
        if (mut_arg0->m_type != cfvariant::Struct) throw webstrada::exception("StructInsert: First argument must be a struct");
        string key = const_cast<cfvariant*>(args[1])->toString();
        bool allowOverwrite = false;
        if (arg_count == 4) {
            allowOverwrite = isTruthy(*args[3]);
        }
        if (!allowOverwrite && mut_arg0->m_struct->contains(key)) throw webstrada::exception("StructInsert: Key already exists");
        mut_arg0->structSet(key, *args[2]);
        return cfvariant_create_bool(true);
    }
    if (fname.equals("STRUCTUPDATE")) {
        if (arg_count != 3) throw webstrada::exception("StructUpdate requires exactly 3 arguments");
        if (mut_arg0->m_type != cfvariant::Struct) throw webstrada::exception("StructUpdate: First argument must be a struct");
        string key = const_cast<cfvariant*>(args[1])->toString();
        if (!mut_arg0->m_struct->contains(key)) throw webstrada::exception("StructUpdate: Key not found");
        mut_arg0->structSet(key, *args[2]);
        return cfvariant_create_bool(true);
    }
    if (fname.equals("STRUCTDELETE")) {
        if (arg_count < 2 || arg_count > 3) throw webstrada::exception("StructDelete requires 2 or 3 arguments");
        if (mut_arg0->m_type != cfvariant::Struct && mut_arg0->m_type != cfvariant::Component) throw webstrada::exception("StructDelete: First argument must be a struct");
        string key = const_cast<cfvariant*>(args[1])->toString(); key.toUpper();
        bool indicateExisting = false;
        if (arg_count == 3) {
            indicateExisting = isTruthy(*args[2]);
        }
        bool existed = mut_arg0->m_struct->contains(key);
        if (existed) {
            struct_data_bump(mut_arg0->m_structData);
            mut_arg0->m_struct->erase(key);
        }
        return cfvariant_create_bool(indicateExisting ? existed : true);
    }
    if (fname.equals("STRUCTCLEAR")) {
        if (arg_count != 1) throw webstrada::exception("StructClear requires exactly 1 argument");
        if (mut_arg0->m_type != cfvariant::Struct) throw webstrada::exception("StructClear: First argument must be a struct");
        struct_data_bump(mut_arg0->m_structData);
        mut_arg0->m_struct->clear();
        return cfvariant_create_bool(true);
    }

    if (fname.equals("STRUCTKEYEXISTS")) {
        if (arg_count != 2) throw webstrada::exception("StructKeyExists requires exactly 2 arguments");
        return cf_structkeyexists(args[0], args[1]);
    }
    if (fname.equals("STRUCTCOUNT")) {
        if (arg_count != 1) throw webstrada::exception("StructCount requires exactly 1 argument");
        return cf_structcount(args[0]);
    }
    if (fname.equals("STRUCTISEMPTY")) {
        if (arg_count != 1) throw webstrada::exception("StructIsEmpty requires exactly 1 argument");
        return cf_structisempty(args[0]);
    }
    if (fname.equals("STRUCTFIND")) {
        if (arg_count != 2) throw webstrada::exception("StructFind requires exactly 2 arguments");
        return cf_structfind(args[0], args[1]);
    }
    if (fname.equals("STRUCTKEYARRAY")) {
        if (arg_count != 1) throw webstrada::exception("StructKeyArray requires exactly 1 argument");
        return cf_structkeyarray(args[0]);
    }
    if (fname.equals("STRUCTKEYLIST")) {
        if (arg_count < 1 || arg_count > 2) throw webstrada::exception("StructKeyList requires 1 or 2 arguments");
        return cf_structkeylist(args[0], (arg_count == 2) ? args[1] : nullptr);
    }
    if (fname.equals("STRUCTVALUEARRAY")) {
        if (arg_count != 1) throw webstrada::exception("StructValueArray requires exactly 1 argument");
        return cf_structvaluearray(args[0]);
    }

    // JSON function dispatches
    if (fname.equals("CANSERIALIZE") || fname.equals("CANDESERIALIZE")) {
        return cfvariant_create_bool(true);
    }

    if (fname.equals("ISJSON")) {
        if (arg_count != 1) throw webstrada::exception("IsJSON requires exactly 1 argument");
        return cf_isjson(args[0]);
    }

    if (fname.equals("SERIALIZEJSON") || fname.equals("SERIALIZE")) {
        if (arg_count < 1) throw webstrada::exception(fname + " requires at least 1 argument");
        if (fname.equals("SERIALIZE")) {
            if (arg_count < 2) throw webstrada::exception("Serialize requires at least 2 arguments");
            return cf_serialize(args[0], args[1]);
        }
        return cf_serializejson(args[0], (arg_count >= 2) ? args[1] : nullptr);
    }

    if (fname.equals("DESERIALIZEJSON") || fname.equals("DESERIALIZE")) {
        if (arg_count < 1) throw webstrada::exception(fname + " requires at least 1 argument");
        if (fname.equals("DESERIALIZE")) {
            if (arg_count < 2) throw webstrada::exception("Deserialize requires at least 2 arguments");
            return cf_deserialize(args[0], args[1]);
        }
        // strictMapping is the (boolean) 2nd argument of DeserializeJSON.
        return cf_deserializejson(args[0], (arg_count >= 2) ? args[1] : nullptr);
    }

    // File function dispatches
    if (fname.equals("FILEOPEN")) {
        if (arg_count < 2 || arg_count > 3) throw webstrada::exception("FileOpen requires 2 or 3 arguments");
        return cf_fileopen(args[0], args[1], (arg_count == 3) ? args[2] : nullptr);
    }
    if (fname.equals("FILECLOSE")) {
        if (arg_count != 1) throw webstrada::exception("FileClose requires exactly 1 argument");
        return cf_fileclose(args[0]);
    }
    if (fname.equals("FILEREADLINE")) {
        if (arg_count != 1) throw webstrada::exception("FileReadLine requires exactly 1 argument");
        return cf_filereadline(args[0]);
    }
    if (fname.equals("FILEWRITELINE")) {
        if (arg_count != 2) throw webstrada::exception("FileWriteLine requires exactly 2 arguments");
        return cf_filewriteline(args[0], args[1]);
    }
    if (fname.equals("FILESEEK")) {
        if (arg_count != 2) throw webstrada::exception("FileSeek requires exactly 2 arguments");
        return cf_fileseek(args[0], args[1]);
    }
    if (fname.equals("FILESKIPBYTES")) {
        if (arg_count != 2) throw webstrada::exception("FileSkipBytes requires exactly 2 arguments");
        return cf_fileskipbytes(args[0], args[1]);
    }
    if (fname.equals("FILEREADBINARY")) {
        if (arg_count != 1) throw webstrada::exception("FileReadBinary requires exactly 1 argument");
        return cf_filereadbinary(args[0]);
    }
    if (fname.equals("BINARYDECODE")) {
        if (arg_count != 2) throw webstrada::exception("BinaryDecode requires exactly 2 arguments");
        return cf_binarydecode(args[0], args[1]);
    }
    if (fname.equals("FILEGETMIMETYPE")) {
        if (arg_count != 1) throw webstrada::exception("FileGetMimeType requires exactly 1 argument");
        return cf_filegetmimetype(args[0]);
    }
    if (fname.equals("FILESETACCESSMODE")) {
        if (arg_count != 2) throw webstrada::exception("FileSetAccessMode requires exactly 2 arguments");
        return cf_filesetaccessmode(args[0], args[1]);
    }
    if (fname.equals("FILESETATTRIBUTE")) {
        if (arg_count != 2) throw webstrada::exception("FileSetAttribute requires exactly 2 arguments");
        return cf_filesetattribute(args[0], args[1]);
    }
    if (fname.equals("FILESETLASTMODIFIED")) {
        if (arg_count != 2) throw webstrada::exception("FileSetLastModified requires exactly 2 arguments");
        return cf_filesetlastmodified(args[0], args[1]);
    }
    if (fname.equals("DIRECTORYCOPY")) {
        if (arg_count != 2) throw webstrada::exception("DirectoryCopy requires exactly 2 arguments");
        return cf_directorycopy(args[0], args[1]);
    }
    if (fname.equals("DIRECTORYRENAME")) {
        if (arg_count != 2) throw webstrada::exception("DirectoryRename requires exactly 2 arguments");
        return cf_directoryrename(args[0], args[1]);
    }
    if (fname.equals("DIRECTORYLIST")) {
        if (arg_count < 1 || arg_count > 5) throw webstrada::exception("DirectoryList requires 1 to 5 arguments");
        return cf_directorylist(args[0], (arg_count >= 2) ? args[1] : nullptr,
            (arg_count >= 3) ? args[2] : nullptr,
            (arg_count >= 4) ? args[3] : nullptr,
            (arg_count >= 5) ? args[4] : nullptr);
    }
    if (fname.equals("EXPANDPATH")) {
        if (arg_count != 1) throw webstrada::exception("ExpandPath requires exactly 1 argument");
        return cf_expandpath(args[0]);
    }
    if (fname.equals("WRITEDUMP")) {
        if (arg_count < 1 || arg_count > 12) throw webstrada::exception("WriteDump requires 1 to 12 arguments");
        return cf_writedump(args[0], (arg_count >= 2) ? args[1] : nullptr,
            (arg_count >= 3) ? args[2] : nullptr, (arg_count >= 4) ? args[3] : nullptr,
            (arg_count >= 5) ? args[4] : nullptr, (arg_count >= 6) ? args[5] : nullptr,
            (arg_count >= 7) ? args[6] : nullptr, (arg_count >= 8) ? args[7] : nullptr,
            (arg_count >= 9) ? args[8] : nullptr, (arg_count >= 10) ? args[9] : nullptr,
            (arg_count >= 11) ? args[10] : nullptr, (arg_count >= 12) ? args[11] : nullptr);
    }
    if (fname.equals("WRITELOG")) {
        if (arg_count < 1 || arg_count > 5) throw webstrada::exception("WriteLog requires 1 to 5 arguments");
        return cf_writelog(args[0], (arg_count >= 2) ? args[1] : nullptr,
            (arg_count >= 3) ? args[2] : nullptr, (arg_count >= 4) ? args[3] : nullptr,
            (arg_count >= 5) ? args[4] : nullptr);
    }
    if (fname.equals("GETPROFILESECTIONS")) {
        if (arg_count != 1) throw webstrada::exception("GetProfileSections requires exactly 1 argument");
        return cf_getprofilesections(args[0]);
    }
    if (fname.equals("GETPROFILESTRING")) {
        if (arg_count != 3) throw webstrada::exception("GetProfileString requires exactly 3 arguments");
        return cf_getprofilestring(args[0], args[1], args[2]);
    }
    if (fname.equals("FILEUPLOAD")) {
        if (arg_count < 1 || arg_count > 5) throw webstrada::exception("FileUpload requires 1 to 5 arguments");
        return cf_fileupload(args[0],
            (arg_count >= 2) ? args[1] : nullptr,
            (arg_count >= 3) ? args[2] : nullptr,
            (arg_count >= 4) ? args[3] : nullptr,
            (arg_count >= 5) ? args[4] : nullptr);
    }
    if (fname.equals("FILEUPLOADALL")) {
        if (arg_count < 1 || arg_count > 7) throw webstrada::exception("FileUploadAll requires 1 to 7 arguments");
        return cf_fileuploadall(static_cast<cfvariant*>(variables), args[0],
            (arg_count >= 2) ? args[1] : nullptr,
            (arg_count >= 3) ? args[2] : nullptr,
            (arg_count >= 4) ? args[3] : nullptr,
            (arg_count >= 5) ? args[4] : nullptr,
            (arg_count >= 6) ? args[5] : nullptr,
            (arg_count >= 7) ? args[6] : nullptr);
    }
    if (fname.equals("HASH")) {
        if (arg_count < 1 || arg_count > 4) throw webstrada::exception("Hash requires 1 to 4 arguments");
        return cf_hash(args[0],
            (arg_count >= 2) ? args[1] : nullptr,
            (arg_count >= 3) ? args[2] : nullptr,
            (arg_count >= 4) ? args[3] : nullptr);
    }
    if (fname.equals("HMAC")) {
        if (arg_count < 2 || arg_count > 4) throw webstrada::exception("HMac requires 2 to 4 arguments");
        return cf_hmac(args[0], args[1],
            (arg_count >= 3) ? args[2] : nullptr,
            (arg_count >= 4) ? args[3] : nullptr);
    }
    if (fname.equals("ENCRYPT") || fname.equals("DECRYPT") || fname.equals("ENCRYPTBINARY") || fname.equals("DECRYPTBINARY")) {
        if (arg_count < 2 || arg_count > 6) throw webstrada::exception(fname + " requires 2 to 6 arguments");
        const cfvariant *a1 = args[0];
        const cfvariant *a2 = args[1];
        const cfvariant *a3 = (arg_count >= 3) ? args[2] : nullptr;
        const cfvariant *a4 = (arg_count >= 4) ? args[3] : nullptr;
        const cfvariant *a5 = (arg_count >= 5) ? args[4] : nullptr;
        const cfvariant *a6 = (arg_count >= 6) ? args[5] : nullptr;
        if (fname.equals("ENCRYPT")) return cf_encrypt(a1, a2, a3, a4, a5, a6);
        if (fname.equals("DECRYPT")) return cf_decrypt(a1, a2, a3, a4, a5, a6);
        if (fname.equals("ENCRYPTBINARY")) return cf_encryptbinary(a1, a2, a3, a4, a5, a6);
        return cf_decryptbinary(a1, a2, a3, a4, a5, a6);
    }
    if (fname.equals("GENERATESECRETKEY")) {
        if (arg_count < 1 || arg_count > 2) throw webstrada::exception("GenerateSecretKey requires 1 or 2 arguments");
        return cf_generatesecretkey(args[0], (arg_count >= 2) ? args[1] : nullptr);
    }
    if (fname.equals("GENERATE3DESKEY")) {
        if (arg_count != 1) throw webstrada::exception("Generate3DesKey requires exactly 1 argument");
        return cf_generate3deskey(args[0]);
    }
    if (fname.equals("GENERATEPBKDFKEY")) {
        if (arg_count != 5) throw webstrada::exception("GeneratePBKDFKey requires exactly 5 arguments");
        return cf_generatepbkdfkey(args[0], args[1], args[2], args[3], args[4]);
    }
    if (fname.equals("TOBASE64")) {
        if (arg_count < 1 || arg_count > 2) throw webstrada::exception("ToBase64 requires 1 or 2 arguments");
        return cf_tobase64(args[0], (arg_count >= 2) ? args[1] : nullptr);
    }
    if (fname.equals("TOBINARY")) {
        if (arg_count != 1) throw webstrada::exception("ToBinary requires exactly 1 argument");
        return cf_tobinary(args[0]);
    }
    if (fname.equals("BINARYENCODE")) {
        if (arg_count != 2) throw webstrada::exception("BinaryEncode requires exactly 2 arguments");
        return cf_binaryencode(args[0], args[1]);
    }
    if (fname.equals("URLDECODE") || fname.equals("URLENCODEDFORMAT")) {
        if (arg_count < 1 || arg_count > 2) throw webstrada::exception(fname + " requires 1 or 2 arguments");
        if (fname.equals("URLDECODE")) return cf_urldecode(args[0], (arg_count >= 2) ? args[1] : nullptr);
        return cf_urlencodedformat(args[0], (arg_count >= 2) ? args[1] : nullptr);
    }
    if (fname.equals("ENCODEFORURL")) {
        if (arg_count < 1 || arg_count > 2) throw webstrada::exception("EncodeForURL requires 1 or 2 arguments");
        return cf_encodeforurl(args[0], (arg_count >= 2) ? args[1] : nullptr);
    }
    if (fname.equals("DECODEFROMURL")) {
        if (arg_count != 1) throw webstrada::exception("DecodeFromURL requires exactly 1 argument");
        return cf_decodefromurl(args[0]);
    }
    if (fname.equals("URLSESSIONFORMAT")) {
        if (arg_count != 1) throw webstrada::exception("URLSessionFormat requires exactly 1 argument");
        return cf_urlsessionformat(args[0]);
    }
    if (fname.equals("APPLICATIONSTOP") || fname.equals("GETAPPLICATIONMETADATA") ||
        fname.equals("SESSIONGETMETADATA") || fname.equals("SESSIONINVALIDATE") ||
        fname.equals("SESSIONROTATE")) {
        if (arg_count != 0) throw webstrada::exception(fname + " requires 0 arguments");
        if (fname.equals("APPLICATIONSTOP")) return cf_applicationstop();
        if (fname.equals("GETAPPLICATIONMETADATA")) return cf_getapplicationmetadata();
        if (fname.equals("SESSIONGETMETADATA")) return cf_sessiongetmetadata();
        if (fname.equals("SESSIONINVALIDATE")) return cf_sessioninvalidate();
        return cf_sessionrotate();
    }
    if (fname.equals("CHARSETDECODE")) {
        if (arg_count != 2) throw webstrada::exception("CharsetDecode requires exactly 2 arguments");
        return cf_charsetdecode(args[0], args[1]);
    }
    if (fname.equals("CHARSETENCODE")) {
        if (arg_count != 2) throw webstrada::exception("CharsetEncode requires exactly 2 arguments");
        return cf_charsetencode(args[0], args[1]);
    }
    if (fname.equals("TOSCRIPT")) {
        if (arg_count < 2 || arg_count > 4) throw webstrada::exception("ToScript requires 2 to 4 arguments");
        return cf_toscript(args[0], args[1],
            (arg_count >= 3) ? args[2] : nullptr,
            (arg_count >= 4) ? args[3] : nullptr);
    }
    if (fname.equals("TOSTRING")) {
        if (arg_count < 1 || arg_count > 2) throw webstrada::exception("ToString requires 1 or 2 arguments");
        return cf_tostring(args[0], (arg_count >= 2) ? args[1] : nullptr);
    }
    if (fname.equals("VAL")) {
        if (arg_count != 1) throw webstrada::exception("Val requires exactly 1 argument");
        return cf_val(args[0]);
    }
    if (fname.equals("HTMLEDITFORMAT")) {
        if (arg_count < 1 || arg_count > 2) throw webstrada::exception("HTMLEditFormat requires 1 or 2 arguments");
        return cf_htmleditformat(args[0], arg_count == 2 ? args[1] : nullptr);
    }
    if (fname.equals("HTMLCODEFORMAT")) {
        if (arg_count < 1 || arg_count > 2) throw webstrada::exception("HTMLCodeFormat requires 1 or 2 arguments");
        return cf_htmlcodeformat(args[0], arg_count == 2 ? args[1] : nullptr);
    }
    if (fname.equals("INVOKE")) {
        if (arg_count < 2 || arg_count > 3) throw webstrada::exception("Invoke requires 2 or 3 arguments");
        return cf_invoke(args[0], args[1], arg_count == 3 ? args[2] : nullptr,
                         out, cgi, server, cookie, application, session, url, form, variables);
    }
    if (fname.equals("AJAXLINK")) {
        if (arg_count != 1) throw webstrada::exception("AjaxLink requires exactly 1 argument");
        return cf_ajaxlink(args[0]);
    }
    if (fname.equals("AJAXONLOAD")) {
        if (arg_count != 1) throw webstrada::exception("AjaxOnLoad requires exactly 1 argument");
        return cf_ajaxonload(args[0], out);
    }
    if (fname.equals("INVOKECFCLIENTFUNCTION")) {
        // Not a CF 2025 function: reproduce the variable-undefined error.
        throw webstrada::exception("Variable INVOKECFCLIENTFUNCTION is undefined.");
    }
    if (fname.equals("REPLACELIST")) {
        if (arg_count < 3 || arg_count > 6) throw webstrada::exception("ReplaceList requires 3 to 6 arguments");
        return cf_replacelist(args[0], args[1], args[2],
                              arg_count >= 4 ? args[3] : nullptr,
                              arg_count >= 5 ? args[4] : nullptr,
                              arg_count == 6 ? args[5] : nullptr);
    }
    if (fname.equals("REESCAPE")) {
        if (arg_count != 1) throw webstrada::exception("ReEscape requires exactly 1 argument");
        return cf_reescape(args[0]);
    }
    if (fname.equals("REFIND") || fname.equals("REFINDNOCASE")) {
        if (arg_count < 2 || arg_count > 5) throw webstrada::exception(fname + " requires 2 to 5 arguments");
        if (fname.equals("REFIND")) {
            return cf_refind(args[0], args[1], arg_count >= 3 ? args[2] : nullptr,
                             arg_count >= 4 ? args[3] : nullptr, arg_count == 5 ? args[4] : nullptr);
        }
        return cf_refindnocase(args[0], args[1], arg_count >= 3 ? args[2] : nullptr,
                               arg_count >= 4 ? args[3] : nullptr, arg_count == 5 ? args[4] : nullptr);
    }
    if (fname.equals("REMATCH") || fname.equals("REMATCHNOCASE")) {
        if (arg_count != 2) throw webstrada::exception(fname + " requires exactly 2 arguments");
        if (fname.equals("REMATCH")) return cf_rematch(args[0], args[1]);
        return cf_rematchnocase(args[0], args[1]);
    }
    if (fname.equals("REREPLACE") || fname.equals("REREPLACENOCASE")) {
        if (arg_count < 3 || arg_count > 4) throw webstrada::exception(fname + " requires 3 or 4 arguments");
        if (fname.equals("REREPLACE")) {
            return cf_rereplace(args[0], args[1], args[2], arg_count == 4 ? args[3] : nullptr);
        }
        return cf_rereplacenocase(args[0], args[1], args[2], arg_count == 4 ? args[3] : nullptr);
    }

    if (fname.equals("CREATEOBJECT")) {
        return cf_createobject(args, arg_count, out, cgi, server, cookie,
                               application, session, url, form, variables);
    }

    if (fname.equals("GETCOMPONENTMETADATA")) {
        if (arg_count != 1) throw webstrada::exception("GetComponentMetaData requires exactly 1 argument");
        return cf_getcomponentmetadata_impl(args[0]);
    }

    if (fname.equals("ISINSTANCEOF")) {
        if (arg_count != 2) throw webstrada::exception("IsInstanceOf requires exactly 2 arguments");
        return cf_isinstanceof_impl(args[0], args[1]);
    }

    // ---- Tier-1 built-in functions (see CFFUNCTION_IMPLEMENTATION_ACTION_PLAN.md) ----

    // No-argument functions.
    if (fname.equals("GETCONTEXTROOT") || fname.equals("GETLOCALHOSTIP") || fname.equals("ISDEBUGMODE") ||
        fname.equals("GETSYSTEMFREEMEMORY") || fname.equals("GETSYSTEMTOTALMEMORY") ||
        fname.equals("GETFUNCTIONLIST") || fname.equals("GETCSPNONCE") || fname.equals("GETCLIENTVARIABLESLIST") ||
        fname.equals("TRANSACTIONCOMMIT")) {
        if (arg_count != 0) throw webstrada::exception(fname + " requires 0 arguments");
        if (fname.equals("GETCONTEXTROOT")) return cf_getcontextroot();
        if (fname.equals("GETLOCALHOSTIP")) return cf_getlocalhostip();
        if (fname.equals("ISDEBUGMODE")) return cf_isdebugmode();
        if (fname.equals("GETSYSTEMFREEMEMORY")) return cf_getsystemfreememory();
        if (fname.equals("GETSYSTEMTOTALMEMORY")) return cf_getsystemtotalmemory();
        if (fname.equals("GETFUNCTIONLIST")) return cf_getfunctionlist();
        if (fname.equals("GETCSPNONCE")) return cf_getcspnonce();
        if (fname.equals("GETCLIENTVARIABLESLIST")) return cf_getclientvariableslist();
        return cf_transactioncommit();
    }

    // Single-argument functions.
    if (fname.equals("GETENCODING") || fname.equals("GETFREESPACE") || fname.equals("GETTOTALSPACE") ||
        fname.equals("ISIPV6") || fname.equals("ISLOCALHOST") || fname.equals("GETMETRICDATA") ||
        fname.equals("ISDDX") || fname.equals("ISWDDX") || fname.equals("DELETECLIENTVARIABLE") ||
        fname.equals("PRESERVESINGLEQUOTES") || fname.equals("CREATEODBCDATE") ||
        fname.equals("CREATEODBCTIME") || fname.equals("ISTHREADINTERRUPTED")) {
        if (arg_count != 1) throw webstrada::exception(fname + " requires exactly 1 argument");
        if (fname.equals("GETENCODING")) return cf_getencoding(args[0]);
        if (fname.equals("GETFREESPACE")) return cf_getfreespace(args[0]);
        if (fname.equals("GETTOTALSPACE")) return cf_gettotalspace(args[0]);
        if (fname.equals("ISIPV6")) return cf_isipv6(args[0]);
        if (fname.equals("ISLOCALHOST")) return cf_islocalhost(args[0]);
        if (fname.equals("GETMETRICDATA")) return cf_getmetricdata(args[0]);
        if (fname.equals("ISDDX")) return cf_isddx(args[0]);
        if (fname.equals("ISWDDX")) return cf_iswddx(args[0]);
        if (fname.equals("DELETECLIENTVARIABLE")) return cf_deleteclientvariable(args[0]);
        if (fname.equals("PRESERVESINGLEQUOTES")) return cf_preservesinglequotes(args[0]);
        if (fname.equals("CREATEODBCDATE")) return cf_createodbcdate(args[0]);
        if (fname.equals("CREATEODBCTIME")) return cf_createodbctime(args[0]);
        return cf_isthreadinterrupted(args[0]);
    }

    if (fname.equals("OBJECTEQUALS")) {
        if (arg_count != 2) throw webstrada::exception("ObjectEquals requires exactly 2 arguments");
        return cf_objectequals(args[0], args[1]);
    }

    if (fname.equals("GETTOKEN")) {
        if (arg_count < 2 || arg_count > 3) throw webstrada::exception("GetToken requires 2 or 3 arguments");
        return cf_gettoken(args[0], args[1], arg_count == 3 ? args[2] : nullptr);
    }

    if (fname.equals("TRANSACTIONROLLBACK")) {
        if (arg_count > 1) throw webstrada::exception("TransactionRollback requires 0 or 1 arguments");
        return cf_transactionrollback(arg_count == 1 ? args[0] : nullptr);
    }

    if (fname.equals("TRANSACTIONSETSAVEPOINT")) {
        if (arg_count != 1) throw webstrada::exception("TransactionSetSavePoint requires exactly 1 argument");
        return cf_transactionsetsavepoint(args[0]);
    }

    if (fname.equals("LOCATION")) {
        return cf_location(arg_count >= 1 ? args[0] : nullptr,
                           arg_count >= 2 ? args[1] : nullptr,
                           arg_count >= 3 ? args[2] : nullptr, arg_count);
    }

    if (fname.equals("SETVARIABLE")) {
        if (arg_count != 2) throw webstrada::exception("SetVariable requires exactly 2 arguments");
        return cf_setvariable(args[0], args[1], cgi, server, cookie,
                              application, session, url, form, variables);
    }

    // ---- Tier-2 encoder family (CFFUNCTION_IMPLEMENTATION_ACTION_PLAN.md) ----
    if (fname.equals("ENCODEFORHTML") || fname.equals("ENCODEFORHTMLATTRIBUTE") || fname.equals("ENCODEFORJAVASCRIPT") ||
        fname.equals("ENCODEFORCSS") || fname.equals("ENCODEFORXML") || fname.equals("ENCODEFORXMLATTRIBUTE") ||
        fname.equals("ENCODEFORDN") || fname.equals("ENCODEFORLDAP") || fname.equals("ENCODEFORXPATH")) {
        if (arg_count < 1 || arg_count > 2) throw webstrada::exception(fname + " requires 1 or 2 arguments");
        if (fname.equals("ENCODEFORHTML")) return cf_encodeforhtml(args[0], arg_count == 2 ? args[1] : nullptr);
        if (fname.equals("ENCODEFORHTMLATTRIBUTE")) return cf_encodeforhtmlattribute(args[0], arg_count == 2 ? args[1] : nullptr);
        if (fname.equals("ENCODEFORJAVASCRIPT")) return cf_encodeforjavascript(args[0], arg_count == 2 ? args[1] : nullptr);
        if (fname.equals("ENCODEFORCSS")) return cf_encodeforcss(args[0], arg_count == 2 ? args[1] : nullptr);
        if (fname.equals("ENCODEFORXML")) return cf_encodeforxml(args[0], arg_count == 2 ? args[1] : nullptr);
        if (fname.equals("ENCODEFORXMLATTRIBUTE")) return cf_encodeforxmlattribute(args[0], arg_count == 2 ? args[1] : nullptr);
        if (fname.equals("ENCODEFORDN")) return cf_encodefordn(args[0], arg_count == 2 ? args[1] : nullptr);
        if (fname.equals("ENCODEFORLDAP")) return cf_encodeforldap(args[0], arg_count == 2 ? args[1] : nullptr);
        return cf_encodeforxpath(args[0], arg_count == 2 ? args[1] : nullptr);
    }

    if (fname.equals("DECODEFORHTML")) {
        if (arg_count != 1) throw webstrada::exception("DecodeForHTML requires exactly 1 argument");
        return cf_decodeforhtml(args[0]);
    }

    if (fname.equals("CANONICALIZE")) {
        if (arg_count < 3 || arg_count > 4) throw webstrada::exception("Canonicalize requires 3 or 4 arguments");
        return cf_canonicalize(args[0], args[1], args[2], arg_count == 4 ? args[3] : nullptr);
    }

    if (fname.equals("NUMBERFORMAT")) {
        if (arg_count < 1 || arg_count > 2) throw webstrada::exception("NumberFormat requires 1 or 2 arguments");
        return cf_numberformat(args[0], arg_count == 2 ? args[1] : nullptr);
    }

    if (fname.equals("DUPLICATE")) {
        if (arg_count != 1) throw webstrada::exception("Duplicate requires exactly 1 argument");
        return cf_duplicate(args[0]);
    }

    if (fname.equals("IIF")) {
        if (arg_count != 3) throw webstrada::exception("IIf requires exactly 3 arguments");
        return cf_iif(args[0], args[1], args[2], out, cgi, server, cookie,
                      application, session, url, form, variables);
    }

    if (fname.equals("ISVALID")) {
        if (arg_count < 2 || arg_count > 5) throw webstrada::exception("IsValid requires 2 to 5 arguments");
        return cf_isvalid(args[0], args[1],
                          arg_count >= 3 ? args[2] : nullptr,
                          arg_count >= 4 ? args[3] : nullptr,
                          arg_count == 5 ? args[4] : nullptr);
    }

    if (fname.equals("GETEXCEPTION")) {
        if (arg_count != 1) throw webstrada::exception("GetException requires exactly 1 argument");
        return cf_getexception(args[0]);
    }

    if (fname.equals("GETLOCALEDISPLAYNAME")) {
        if (arg_count > 2) throw webstrada::exception("GetLocaleDisplayName requires 0 to 2 arguments");
        return cf_getlocaledisplayname(arg_count >= 1 ? args[0] : nullptr,
                                       arg_count == 2 ? args[1] : nullptr);
    }

    if (fname.equals("GETCPUUSAGE")) {
        if (arg_count > 1) throw webstrada::exception("GetCPUUsage requires 0 or 1 arguments");
        return cf_getcpuusage(arg_count == 1 ? args[0] : nullptr);
    }

    if (fname.equals("GETMETADATA")) {
        if (arg_count != 1) throw webstrada::exception("GetMetaData requires exactly 1 argument");
        return cf_getmetadata(args[0]);
    }

    if (fname.equals("SETPROFILESTRING")) {
        if (arg_count < 4 || arg_count > 5) throw webstrada::exception("SetProfileString requires 4 or 5 arguments");
        return cf_setprofilestring(args[0], args[1], args[2], args[3],
                                   arg_count == 5 ? args[4] : nullptr);
    }

    if (fname.equals("GETPROPERTYSTRING")) {
        if (arg_count < 2 || arg_count > 3) throw webstrada::exception("GetPropertyString requires 2 or 3 arguments");
        return cf_getpropertystring(args[0], args[1], arg_count == 3 ? args[2] : nullptr);
    }

    if (fname.equals("GETPROPERTYFILE")) {
        if (arg_count < 1 || arg_count > 2) throw webstrada::exception("GetPropertyFile requires 1 or 2 arguments");
        return cf_getpropertyfile(args[0], arg_count == 2 ? args[1] : nullptr);
    }

    if (fname.equals("SETPROPERTYSTRING")) {
        if (arg_count < 2 || arg_count > 4) throw webstrada::exception("SetPropertyString requires 2 to 4 arguments");
        return cf_setpropertystring(args[0], args[1],
                                    arg_count >= 3 ? args[2] : nullptr,
                                    arg_count == 4 ? args[3] : nullptr);
    }

    if (fname.equals("CSRFGENERATETOKEN")) {
        if (arg_count > 2) throw webstrada::exception("CSRFGenerateToken requires 0 to 2 arguments");
        return cf_csrfgeneratetoken(arg_count >= 1 ? args[0] : nullptr,
                                    arg_count == 2 ? args[1] : nullptr);
    }

    if (fname.equals("CSRFVERIFYTOKEN")) {
        if (arg_count < 1 || arg_count > 2) throw webstrada::exception("CSRFVerifyToken requires 1 or 2 arguments");
        return cf_csrfverifytoken(args[0], arg_count == 2 ? args[1] : nullptr);
    }

    if (fname.equals("ISONLINE")) {
        if (arg_count != 1) throw webstrada::exception("isOnline requires exactly 1 argument");
        return cf_isonline(args[0]);
    }

    if (fname.equals("ISPDFFILE") || fname.equals("ISPDFOBJECT")) {
        if (arg_count != 1) throw webstrada::exception(fname + " requires exactly 1 argument");
        return fname.equals("ISPDFFILE") ? cf_ispdffile(args[0]) : cf_ispdfobject(args[0]);
    }

    if (fname.equals("ISPDFARCHIVE")) {
        if (arg_count < 1 || arg_count > 2) throw webstrada::exception("IsPDFArchive requires 1 or 2 arguments");
        return cf_ispdfarchive(args[0], arg_count == 2 ? args[1] : nullptr);
    }

    if (fname.equals("CSVREAD")) {
        if (arg_count < 1 || arg_count > 4) throw webstrada::exception("CSVRead requires 1 to 4 arguments");
        return cf_csvread(args[0],
                          arg_count >= 2 ? args[1] : nullptr,
                          arg_count >= 3 ? args[2] : nullptr,
                          arg_count == 4 ? args[3] : nullptr);
    }

    if (fname.equals("CSVWRITE")) {
        if (arg_count < 1 || arg_count > 2) throw webstrada::exception("CSVWrite requires 1 or 2 arguments");
        return cf_csvwrite(args[0], arg_count == 2 ? args[1] : nullptr);
    }

    if (fname.equals("CSVPROCESS")) {
        if (arg_count < 2 || arg_count > 3) throw webstrada::exception("CSVProcess requires 2 or 3 arguments");
        return cf_csvprocess(args[0], args[1], arg_count == 3 ? args[2] : nullptr,
                             out, cgi, server, cookie, application, session, url, form, variables);
    }

    if (fname.equals("OBJECTSAVE")) {
        if (arg_count < 1 || arg_count > 2) throw webstrada::exception("ObjectSave requires 1 or 2 arguments");
        return cf_objectsave(args[0], arg_count == 2 ? args[1] : nullptr);
    }

    if (fname.equals("OBJECTLOAD")) {
        if (arg_count != 1) throw webstrada::exception("ObjectLoad requires exactly 1 argument");
        return cf_objectload(args[0]);
    }

    if (fname.equals("TRACE")) {
        // trace([var], [text], [type], [category], [inline], [abort]) — the
        // <cftrace> tag as a function. Named arguments arrive as the marker
        // struct at args[0]; positional arguments are rejected by CF.
        if (arg_count == 0) {
            return cf_trace(nullptr);
        }
        if (arg_count == 1 && args[0]->m_type == cfvariant::Struct && args[0]->m_struct) {
            return cf_trace(args[0]);
        }
        throw webstrada::exception("Attribute validation error for trace.");
    }

    // ---- Cache family (sqlite-backed) ----
    if (fname.equals("CACHEGET")) {
        if (arg_count < 1 || arg_count > 2) throw webstrada::exception("CacheGet requires 1 or 2 arguments");
        return cf_cacheget(args[0], arg_count == 2 ? args[1] : nullptr);
    }
    if (fname.equals("CACHEPUT")) {
        if (arg_count < 2 || arg_count > 6) throw webstrada::exception("CachePut requires 2 to 6 arguments");
        return cf_cacheput(args[0], args[1],
                           arg_count >= 3 ? args[2] : nullptr,
                           arg_count >= 4 ? args[3] : nullptr,
                           arg_count >= 5 ? args[4] : nullptr,
                           arg_count == 6 ? args[5] : nullptr);
    }
    if (fname.equals("CACHEGETALLIDS")) {
        if (arg_count > 2) throw webstrada::exception("CacheGetAllIds requires 0 to 2 arguments");
        return cf_cachegetallids(arg_count >= 1 ? args[0] : nullptr,
                                 arg_count == 2 ? args[1] : nullptr);
    }
    if (fname.equals("CACHEGETMETADATA")) {
        if (arg_count < 1 || arg_count > 3) throw webstrada::exception("CacheGetMetadata requires 1 to 3 arguments");
        return cf_cachegetmetadata(args[0], arg_count >= 2 ? args[1] : nullptr,
                                   arg_count == 3 ? args[2] : nullptr);
    }
    if (fname.equals("CACHEGETPROPERTIES")) {
        if (arg_count > 1) throw webstrada::exception("CacheGetProperties requires 0 or 1 arguments");
        return cf_cachegetproperties(arg_count == 1 ? args[0] : nullptr);
    }
    if (fname.equals("CACHEGETSESSION")) {
        if (arg_count < 1 || arg_count > 2) throw webstrada::exception("CacheGetSession requires 1 or 2 arguments");
        return cf_cachegetsession(args[0], arg_count == 2 ? args[1] : nullptr);
    }
    if (fname.equals("CACHEIDEXISTS")) {
        if (arg_count < 1 || arg_count > 2) throw webstrada::exception("CacheIdExists requires 1 or 2 arguments");
        return cf_cacheidexists(args[0], arg_count == 2 ? args[1] : nullptr);
    }
    if (fname.equals("CACHEREGIONEXISTS")) {
        if (arg_count != 1) throw webstrada::exception("CacheRegionExists requires exactly 1 argument");
        return cf_cacheregionexists(args[0]);
    }
    if (fname.equals("CACHEREGIONNEW")) {
        if (arg_count < 1 || arg_count > 3) throw webstrada::exception("CacheRegionNew requires 1 to 3 arguments");
        return cf_cacheregionnew(args[0], arg_count >= 2 ? args[1] : nullptr,
                                 arg_count == 3 ? args[2] : nullptr);
    }
    if (fname.equals("CACHEREGIONREMOVE")) {
        if (arg_count != 1) throw webstrada::exception("CacheRegionRemove requires exactly 1 argument");
        return cf_cacheregionremove(args[0]);
    }
    if (fname.equals("CACHEREMOVE")) {
        if (arg_count < 1 || arg_count > 4) throw webstrada::exception("CacheRemove requires 1 to 4 arguments");
        return cf_cacheremove(args[0], arg_count >= 2 ? args[1] : nullptr,
                              arg_count >= 3 ? args[2] : nullptr,
                              arg_count == 4 ? args[3] : nullptr);
    }
    if (fname.equals("CACHEREMOVEALL")) {
        if (arg_count > 1) throw webstrada::exception("CacheRemoveAll requires 0 or 1 arguments");
        return cf_cacheremoveall(arg_count == 1 ? args[0] : nullptr);
    }
    if (fname.equals("CACHESETPROPERTIES")) {
        if (arg_count < 1 || arg_count > 2) throw webstrada::exception("CacheSetProperties requires 1 or 2 arguments");
        return cf_cachesetproperties(args[0], arg_count == 2 ? args[1] : nullptr);
    }
    if (fname.equals("REMOVECACHEDQUERY")) {
        if (arg_count < 1 || arg_count > 4) throw webstrada::exception("RemoveCachedQuery requires 1 to 4 arguments");
        return cf_removecachedquery(args[0], arg_count >= 2 ? args[1] : nullptr,
                                    arg_count >= 3 ? args[2] : nullptr,
                                    arg_count == 4 ? args[3] : nullptr);
    }

    // For all other non-mutating functions, use the JIT argument delegation strategy
    // A call to an unknown function name is resolved as a variable reference in CF,
    // so it fails with "Variable NAME is undefined." (a registered UDF would have
    // been caught by the variable lookup at the top of this function).
    if (!isKnownFunctionName(fname)) {
        throw webstrada::exception(string("Variable ") + fname + " is undefined.");
    }
    cfvariant *vars = static_cast<cfvariant*>(variables);
    if (!vars && !g_udfCtx.empty()) {
        const UdfCallCtx &ctx = g_udfCtx.back();
        vars = ctx.component ? ctx.component->variablesScope : ctx.localScope;
    }
    if (!vars || vars->m_type != cfvariant::Struct) {
        throw webstrada::exception("Function execution error: invalid variables scope");
    }

    for (int i = 0; i < arg_count; i++) {
        char keyBuf[64];
        std::snprintf(keyBuf, sizeof(keyBuf), "__JIT_ARG_%d", i);
        string key(keyBuf);
        vars->set(key) = *args[i];
    }

    string expr = string(name) + "(";
    for (int i = 0; i < arg_count; i++) {
        if (i > 0) expr += ",";
        char argBuf[64];
        std::snprintf(argBuf, sizeof(argBuf), "__JIT_ARG_%d", i);
        expr += argBuf;
    }
    expr += ")";

    cfvariant res = evaluateExpr(out, expr, cgi, server, cookie, application, session, url, form, vars);

    for (int i = 0; i < arg_count; i++) {
        char keyBuf[64];
        std::snprintf(keyBuf, sizeof(keyBuf), "__JIT_ARG_%d", i);
        string key(keyBuf);
        struct_data_bump(vars->m_structData);
        vars->m_struct->erase(key);
    }

    auto *ret = new cfvariant(res);
    return ret;
}

int cfml::cfvariant_is_truthy(const cfvariant *v) {
    if (!v) return 0;
    return isTruthy(*v);
}

int cfml::cfvariant_to_int(const cfvariant *v) {
    if (!v) return 0;
    return getIntValue(*v);
}

long long cfml::cfvariant_to_long(const cfvariant *v) {
    if (!v) return 0;
    switch (v->m_type) {
        case cfvariant::Number:  return static_cast<long long>(v->m_int);
        case cfvariant::Long:    return v->m_long;
        case cfvariant::Float:   return static_cast<long long>(v->m_double);
        case cfvariant::Boolean: return v->m_bool ? 1 : 0;
        case cfvariant::Array:
            // A query-column reference converts as its current row's cell.
            if (v->m_queryColOwner && v->m_queryColIndex >= 0) {
                cfvariant cell = scalarizeQueryColumn(v);
                return cfvariant_to_long(&cell);
            }
            return 0;
        case cfvariant::String:
            if (!v->m_str) return 0;
            return strtoll(v->m_str->constData(), nullptr, 10);
        default: return 0;
    }
}
