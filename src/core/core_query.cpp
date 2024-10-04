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
// ---- Query-column reference helpers ----
static cfvariant *assignQueryColumn(cfvariant *query, const string &memberPath, const cfvariant *value);

// Resolves a member of a query value to a stable variant. Pseudo-properties
// (columnlist/recordcount/currentrow) return a temp registered with
// that q.a[1] (compiled as cfvariant_get_var("q.a") + cfvariant_index) works.
cfvariant *resolveQueryMember(cfvariant *query, const char *key)
{
    string k(key);
    k.toUpper();
    if (k.equals("COLUMNLIST")) {
        auto *ret = new cfvariant(queryColumnList(query));
        cf_register_temp(ret);
        return ret;
    }
    if (k.equals("RECORDCOUNT")) {
        auto *ret = new cfvariant(queryRecordCount(query));
        cf_register_temp(ret);
        return ret;
    }
    if (k.equals("CURRENTROW")) {
        auto *ret = new cfvariant(query->m_query->currentRow);
        cf_register_temp(ret);
        return ret;
    }
    int colIdx = query->m_query->findColumn(k);
    if (colIdx >= 0) {
        auto *ret = new cfvariant(cfvariant::Array);
        for (auto &v : query->m_query->columns[colIdx].values) {
            ret->m_array->push_back(v);
        }
        // Carry a back-reference so q.a[1] = "z" writes through to the query
        // cell via cfvariant_index_assign. The reference is retained so the
        // query stays alive while any column copy exists. Dot access produces
        // a column whose *copies* are read-only (CF: x = q.a; x[1] = "MUT"
        // throws an Expression error); the temp itself stays writable so the
        // direct q.a[1] = "z" form still writes through.
        ret->m_queryColOwner = query_data_retain(query->m_query);
        ret->m_queryColIndex = colIdx;
        ret->m_queryColFromBracket = false;
        ret->m_queryColWritable = true;
        cf_register_temp(ret);
        return ret;
    }
    return nullptr;
}

// --- Query-column reference helpers ---
// A materialized query column (q.a / q["a"]) is an Array carrying the back-
// reference fields declared in cfvariant.h. These helpers implement the CF
// 2021/2025 semantics for stored copies of such references (verified on the
// RDS host): the first store of a bracket column keeps a live, writable
// reference; a dot-origin column or any second store degrades to the scalar
// value of the column's first cell.

bool cfml::isQueryColumnRef(const cfvariant *v)
{
    return v && v->m_queryColOwner != nullptr;
}

// Java class name ColdFusion reports for a scalar variant's type in runtime
// error messages ("dereference a scalar variable of type class java.X ...",
// "Object of type class java.X cannot be used as an array").
string scalarJavaTypeName(const cfvariant *v)
{
    if (!v) return "java.lang.String";
    switch (v->m_type) {
    case cfvariant::Number:   return "java.lang.Integer";
    case cfvariant::Long:     return "java.lang.Long";
    case cfvariant::Float:    return "java.lang.Double";
    case cfvariant::Boolean:  return "java.lang.Boolean";
    case cfvariant::DateTime: return "java.sql.Date";
    case cfvariant::String:   return "java.lang.String";
    default: break;
    }
    return "java.lang.String";
}

// The scalar value a column reference degrades to when copied a second time
// (or stored from dot access), and what a live column reference stringifies
// as. This is the cell at the query's current cursor position (the first row
// when the cursor has not moved — matching CF 2021's `x = q["a"]; #x#` first-
// cell behavior, and the current row inside a <cfloop query> — verified
// against CF 2025). Null for an empty column.
cfvariant cfml::queryColumnFirstCell(const cfvariant *ref)
{
    if (!ref || !ref->m_queryColOwner) return cfvariant(cfvariant::Null);
    QueryData *qd = ref->m_queryColOwner;
    int colIdx = ref->m_queryColIndex;
    if (colIdx >= 0 && colIdx < (int)qd->columns.size() && !qd->columns[colIdx].values.empty()) {
        int row = qd->currentRow;
        if (row < 1) row = 1;
        if (row <= (int)qd->columns[colIdx].values.size()) {
            return qd->columns[colIdx].values[row - 1];
        }
        return cfvariant(cfvariant::Null);
    }
    return cfvariant(cfvariant::Null);
}

// Store-site handling of a column reference being written into a variable or
// container slot. The depth starts at 0 for a freshly materialized temp; the
// first store of a bracket column (depth 1) keeps the live reference, while a
// dot-origin column or any second store (depth >= 2) degrades to the scalar
// first cell — CF throws "dereference a scalar variable of type class java.X
// as a structure with members." when that scalar is then indexed.
void storeQueryColumnRef(cfvariant &slot)
{
    if (!slot.m_queryColOwner) return;
    slot.m_queryColCopyDepth++;
    if (slot.m_queryColCopyDepth >= 2 || !slot.m_queryColFromBracket) {
        cfvariant scalar = queryColumnFirstCell(&slot);
        slot = scalar;
    }
}

// In a scalar value context (comparison, arithmetic, truthiness), a query-
// column reference (q.id materialized as an Array) behaves like its current
// row's cell — CF's `q.id EQ 2` inside a <cfloop query> compares the scalar,
// not the whole column. Returns the caller's value unchanged unless it is a
// query-column Array reference, in which case the current cell is returned.
cfvariant scalarizeQueryColumn(const cfvariant *v)
{
    if (v && v->m_type == cfvariant::Array && v->m_queryColOwner && v->m_queryColIndex >= 0) {
        return queryColumnFirstCell(v);
    }
    return v ? *v : cfvariant(cfvariant::Null);
}

// Whether v is a plain array for CF's array functions. A column reference only
// counts as an array when it is a freshly materialized bracket-access temp
// (q["a"]); a dot-access temp and any stored copy are not arrays in CF 2021
// (verified on the RDS host: ArrayLen(q["a"]) is 3 but ArrayLen(x) for
// x = q["a"] throws "Object of type class java.lang.String cannot be used as
// an array", and IsArray(x) is NO).
bool cfml::isCfArray(const cfvariant *v)
{
    if (!v || v->m_type != cfvariant::Array) return false;    if (isQueryColumnRef(v)) {
        return v->m_queryColCopyDepth == 0 && v->m_queryColFromBracket;
    }
    // An XmlNodeArray (multi same-name child group) is a Java List, not a real
    // CF array: IsArray() reports NO (verified on the RDS host).
    if (v->m_isXmlNodeList) return false;
    return true;
}

// CF's rejection error for a value that is not a real array, with the value's
// Java type embedded (type "class java.X", message "Object of type class
// java.X cannot be used as an array").
void cfml::throwNotArrayError(const cfvariant *v)
{
    string jt = isQueryColumnRef(v)
        ? queryColJavaTypeName(v->m_queryColOwner, v->m_queryColIndex, 0)
        : scalarJavaTypeName(v);
    throw webstrada::exception("class " + jt, "Object of type class " + jt + " cannot be used as an array", "");
}

// A mutating array function called on an XmlNodeArray (a multi same-name child
// group of an XML element) throws CF's exact message: the Java List CF wraps it
// in is not a real ColdFusion array, so mutations are unsupported (verified on
// the RDS host: arrayAppend(xmlRoot.CHILD, ...) -> "The ArrayAppend ColdFusion
// function is not supported on this object.").
void cfml::throwXmlNodeListUnsupported(const char *fn)
{
    webstrada::string msg("The ");
    msg.append(fn);
    msg.append(" ColdFusion function is not supported on this object.");
    throw webstrada::exception(msg);
}

