#pragma once

#include "string.h"

#include <vector>
#include <map>
#include <string>
#include <cstdint>

#include <strings.h>


namespace webstrada
{

struct UDFInfo;
struct QueryData;
struct ImageData;
struct ComponentInstance;
struct ComponentInfo;

// Case-insensitive ordering for struct keys. ColdFusion structs treat keys
// case-insensitively for lookup but preserve the casing of the first
// insertion; the map stores that original casing while this comparator keeps
// lookups (which historically pass upper-cased keys) working.
struct CiLess
{
    bool operator()(const string &a, const string &b) const
    {
        const char *pa = a.constData();
        const char *pb = b.constData();
        if (!pa) return pb != nullptr;
        if (!pb) return false;
        return strcasecmp(pa, pb) < 0;
    }
};


// Refcounted payload shared by all aliases of a Struct/Xml/JSon/Component
// value (ColdFusion structs are reference types). Owns the key/value map, the
// key insertion-order vector used by SerializeJSON and the StructSetMetadata
// sidecar; a cfvariant's m_struct / m_structInsertOrder point into here and
// copies simply retain the block instead of deep-cloning.
struct StructData
{
    std::map<string, cfvariant, CiLess> map;
    std::vector<string> insertOrder;
    cfvariant *meta = nullptr;   // StructSetMetadata sidecar, owned
    int refs = 1;
    // Bumped whenever a key is erased or the map cleared. Compile-time-bound
    // variable slots (see cfvariant_get_var_fast) memoize a pointer into this
    // map together with the generation captured at memoization time; a fast
    // read is only honored when the generation still matches, so a
    // StructDelete/StructClear on the scope (which frees the node) cannot be
    // read through a dangling pointer. Inserts and reassignments keep the node
    // address stable (std::map guarantees), so they do not bump it.
    uint64_t generation = 0;
};

// Reference-counting helpers for StructData / QueryData payloads.
StructData *struct_data_retain(StructData *sd);
void struct_data_release(StructData *sd);
QueryData *query_data_retain(QueryData *qd);
void query_data_release(QueryData *qd);

// Marks a StructData as structurally mutated (a key erased or the map cleared).
// Cached variable slots memoize the generation, so bumping it invalidates them
// and forces a full scope re-lookup (see cfvariant_get_var_fast).
inline void struct_data_bump(StructData *sd) {
    if (sd) ++sd->generation;
}

class cfvariant
{
public:
    enum cfvariantType
    {
        NotSet,
        Null,
        Boolean,
        Number,
        Long,
        Float,
        String,
        DateTime,
        Array,
        Struct,
        Binary,
        Image,
        Query,
        Xml,
        JSon,
        File,
        Component,
        Function
    };

    cfvariant(bool upcase = true, bool autocreate = false, bool m_readOnly = false, bool m_disabled = false);
    cfvariant(int num);
    cfvariant(const char *str);
    cfvariant(const string &str);
    cfvariant(cfvariantType type);
    virtual ~cfvariant();

    cfvariant(const cfvariant &other);
    cfvariant(cfvariant&& other) noexcept;
    cfvariant& operator=(const cfvariant& other);
    cfvariant& operator=(cfvariant&& other) noexcept;

    // Recursively independent copy (StructCopy / Duplicate). Ordinary copy and
    // assignment share Struct/Query payloads (CF reference semantics); this
    // clones every nested struct/array/query so the result shares nothing.
    cfvariant deepCopy() const;

    cfvariant &operator[](const int index) const;
    cfvariant &operator[](const char *key);
    cfvariant &operator[](const string key);
    cfvariant &operator[](const cfvariant &key) const;

    bool operator==(const string &other) const;
    bool operator==(const char *other) const;

    cfvariant &set(const int index);
    cfvariant &set(const char *key);
    cfvariant &set(const string &key);
    cfvariant &set(const cfvariant &key);

    // Insert or assign a value for a struct/xml key, tracking insertion order
    // (used by SerializeJSON to reproduce ColdFusion's Java HashMap iteration
    // order). Lookup stays case-insensitive; a new key keeps its original case.
    cfvariant &structSet(const string &key, const cfvariant &value);

    void set_type(const cfvariantType type);
    void setUpcase(bool upcase);
    void setAutoCreate();
    void setReadOnly();
    void setDisabled();

    void insert(const cfvariant &item);
    string join(char separator) const;

    bool has(const char *key) const;
    const string toString();

    cfvariantType m_type = NotSet;
    bool m_upcase = true;
    bool m_autocreate = false;
    bool m_readOnly = false;
    bool m_disabled = false;
    union {
        bool                         m_bool;
        int                          m_int;
        long long                    m_long;
        double                       m_double;
        void                        *m_obj = nullptr;
        string                      *m_str;
        std::vector<cfvariant>      *m_array;
        std::map<string, cfvariant, CiLess> *m_struct;
        std::vector<std::byte>      *m_binary;
        int                          m_fd;
    };

    // True when a Boolean came from a source literal (true/false/yes/no) rather
    // than a computed result (comparison, not, boolean-returning function, and/or
    // operand that was computed). CF stringifies literal booleans as true/false
    // but computed ones as YES/NO (see BUGS.md #7). The flag is preserved
    // through copies so a variable holding a literal still stringifies
    // literally. Only Boolean uses this; ignored for other types.
    bool m_boolLiteral = false;

    // True when this exact heap object is currently registered in the request's
    // temp-variant cleanup list (g_temp_variants), so cf_register_temp is
    // idempotent for a given object (fresh results may reach it through several
    // ownership paths). Never copied: it tracks list membership of this object,
    // not value, so copies and assignments reset it to false.
    bool m_tempRegistered = false;

    // Original literal text of a Float value (e.g. "8.0", "1.2300", "5.0E2").
    // Null for computed floats. CF preserves the literal text of float
    // literals (BigDecimal-like storage); any arithmetic/function result is a
    // plain double rendered with the computed formatter. Only Float uses this.
    string *m_literalText = nullptr;

    // Real UDF/closure payload; null when the Function is a plain built-in
    // method handle (whose display text lives in m_str).
    UDFInfo *m_udf = nullptr;

    // The `arguments` scope of a running function: a Struct that additionally
    // carries positional keys ("1".."N") for every passed argument. The
    // introspection helpers (ArrayLen, StructCount, StructKeyList, ...) hide
    // the positional keys that duplicate parameter names so the visible keys
    // match CF (param names + numeric keys for extra args). Only Struct uses
    // these; ignored otherwise.
    bool m_isArguments = false;
    int m_argumentsParamCount = 0;

    // When true, SerializeJSON emits a Struct's keys in insertion order
    // (LinkedHashMap behavior) instead of ColdFusion's default Java HashMap
    // bucket order. Set on structs that wrap Java beans/live objects in CF
    // (e.g. ImageInfo's colormodel for PackedColorModel), whose iteration
    // order CF preserves as insertion order. Only Struct uses this.
    bool m_serializeInsertOrder = false;

    // When true, this Array is a ColdFusion XmlNodeArray (a Java List wrapping
    // the live XML child nodes) rather than a real CF array: IsArray() reports
    // NO, ArrayLen()/len()/indexing still work, and mutating array functions
    // (ArrayAppend/ArrayPrepend/...) throw "not supported on this object". Set
    // on the same-name child groups of an XML element (root.CHILD when the
    // element has several <Child> children; a single occurrence is exposed as
    // the element itself). Only Array uses this.
    bool m_isXmlNodeList = false;

    // When true, this Struct is a captured CFML exception (`cfcatch`) that was
    // raised by a user `throw`/`<cfthrow>` (custom type) rather than by the
    // engine. Preserved through capture→rethrow so `<cfrethrow>` keeps the
    // exact-name matching semantics (see webstrada::exception::m_isCustom).
    bool m_isCustomException = false;

    // When true, this Struct is a captured exception that was an uncatchable
    // abort (`<cfabort>` / `<cflocation>` — abort_exception). Preserved through
    // capture→rethrow so the unmatched path of a `<cftry>` re-raises an
    // abort_exception (keeping it uncatchable) instead of a catchable Request
    // exception.
    bool m_isAbort = false;

    // When true, this Binary value models ColdFusion's ByteArrayOutputStream
    // (cfhttp getasbinary="no" with a non-text MIME body): IsBinary reports NO,
    // Len returns the raw byte count, and stringifying decodes the bytes as
    // UTF-8 with U+FFFD for invalid sequences (Java `new String(byte[])`).
    bool m_isByteArrayOutputStream = false;

    // ODBC rendering style of a DateTime (CreateODBCDate / CreateODBCTime).
    // 0 = normal `{ts 'yyyy-mm-dd hh:nn:ss'}`; 1 = `{d 'yyyy-mm-dd'}` (date
    // only); 2 = `{t 'hh:nn:ss'}` (time only); 3 = CF QueryTable java.util.Date
    // rendering `MM/dd/yyyy hh:nn:ss tt` (cfdirectory dateLastModified). Only
    // DateTime uses this.
    int m_odbcStyle = 0;

// Insertion order of struct keys (parallel to m_struct), for reproducing
// ColdFusion's Java HashMap iteration order in SerializeJSON. Null when
// the value is not a struct/xml container.
std::vector<string> *m_structInsertOrder = nullptr;

// Query payload (m_type == Query). Owned by the variant. Holds the ordered
// column definitions and the cell data in column-major form.
QueryData *m_query = nullptr;

// Reference-counted payload shared by every alias of a Struct/Xml/JSon/
// Component value (ColdFusion structs are reference types: assignment and
// argument passing share one object, so mutations made through any alias are
// visible through every other). The variant's m_struct / m_structInsertOrder
// are borrowed pointers into this block. Null when the value is not a
// struct-like container.
StructData *m_structData = nullptr;

// Image payload (m_type == Image). Shared/refcounted: copies retain the same
// ImageData (CF images share their pixels on assignment).
ImageData *m_image = nullptr;

// Component payload (m_type == Component). Retained ComponentInstance holding
// the compiled definition (ComponentInfo), the this-scope and the variables
// scope. The this scope is also exposed through m_structData/m_struct (the
// instance owns that StructData), so struct introspection, SerializeJSON and
// member access work on the public members. Copies retain the same instance
// (CF objects are reference types).
ComponentInstance *m_component = nullptr;
ComponentInfo *m_superTargetInfo = nullptr;

// Query-column back-reference used while a query column is materialized as an
// Array (q.a / q["a"]). Such a "column reference" reads and writes through to
// the owning query's cell vector (live reads, write-through for in-range rows),
// is not a real array for ArrayLen/ArrayAppend/IsArray once stored in a
// variable, and retains its QueryData so the query stays alive while any
// reference exists (like a CF QueryColumn holding its query). Verified against
// CF 2021/2025 on the RDS host:
//
//   * `x = q["a"]; x[1] = "MUT"` writes through to the query (live reference).
//   * `x = q["a"]; q["a"][2] = "YY"` is visible through x[2] (live read).
//   * `x = q.a; x[1] = "MUT"` throws CF's "dereference a scalar variable of
//     type class java.X as a structure with members." Expression error.
//   * a second copy `y = x` degrades to the scalar first cell (CF 2021), so
//     `y[1] = "CPY"` throws the same Expression error.
//   * writing past the last row (`x[4] = "w"` on a 3-row query) is invisible
//     (reads back empty, recordcount unchanged), matching CF.
//
// m_queryColCopyDepth counts how many times the reference has been stored into
// a variable/slot (0 = freshly materialized temp). The first store of a
// bracket column keeps the live reference; a dot-origin column or any second
// store degrades it to the scalar first cell (see storeQueryColumnRef in
// cf8.cpp). ArrayLen/ArrayAppend/IsArray reject stored references (depth >= 1)
// exactly like CF, while a fresh temp (depth 0) still looks like an array.
QueryData *m_queryColOwner = nullptr;   // retained (see query_data_retain)
int m_queryColIndex = -1;
bool m_queryColFromBracket = false;     // origin: q["a"] vs q.a (copies keep it)
bool m_queryColWritable = true;         // copies: = m_queryColFromBracket
int m_queryColCopyDepth = 0;            // stores into a variable/slot so far
};

// Column-major cell storage for a Query variant. Column names are stored
// uppercased; CF queries treat column lookups case-insensitively. Row access
// is by 1-based index into each column's value vector.
struct QueryColumn
{
    string name;                      // original case (lookups are case-insensitive)
    string type;                      // "varchar", "integer", ...
    std::vector<cfvariant> values;    // one entry per row
};

struct QueryData
{
    std::vector<QueryColumn> columns; // insertion order
    int currentRow = 1;

    // Reference count: queries are reference types in ColdFusion, so copies
    // share this payload (query_data_retain / query_data_release).
    int refs = 1;

    // Explicit row count. Kept in sync with the column value vectors; it is
    // authoritative even when the query has zero columns (CF counts rows added
    // to a column-less query).
    int m_rowCount = 0;

    int rowCount() const
    {
        return m_rowCount;
    }

    // Case-insensitive column lookup; returns index or -1. Safely handles a
    // key whose buffer is null (empty strings in this engine have a null
    // constData()).
    int findColumn(const string &name) const
    {
        const char *key = name.constData();
        if (!key) {
            // Empty key never matches a column.
            return -1;
        }
        for (size_t i = 0; i < columns.size(); i++) {
            if (columns[i].name.compareCaseInsensitive(key) == 0) return static_cast<int>(i);
        }
        return -1;
    }

    // Appends one empty row to every column.
    void addEmptyRow()
    {
        for (auto &col : columns) col.values.push_back(cfvariant(cfvariant::Null));
        m_rowCount++;
    }

    // Appends a row from per-column values (coerced by the caller). Values
    // beyond the declared columns are ignored; missing columns become null.
    void addRow(const std::vector<cfvariant> &values)
    {
        for (size_t c = 0; c < columns.size(); c++) {
            if (c < values.size()) columns[c].values.push_back(values[c]);
            else columns[c].values.push_back(cfvariant(cfvariant::Null));
        }
        m_rowCount++;
    }
};

// One declared parameter of a UDF/closure, as needed by introspection (cfdump
// renders the arguments table from this). `type` is "" when untyped (renders as
// "Any"); `defaultValue` is the evaluated display text of a default literal
// ("" when the parameter has no default).
struct UdfParamInfo
{
    string name;
    string type;
    string defaultValue;
};

// Serialized metadata of a UDF/closure for introspection. `returnType` is ""
// when untyped (renders as "Any"); `access` is e.g. "public".
struct UdfMetaInfo
{
    std::vector<UdfParamInfo> params;
    string returnType;
    string access;
};

// Binary serialization of UdfMetaInfo. The compiler embeds the blob as a
// global constant and hands it to cfvariant_create_udf, which deserializes it
// into the UDFInfo payload. Format (all little-endian): u32 totalLen (bytes
// that follow), u32 paramCount, then per param {u32 nameLen, name, u32 typeLen,
// type, u32 defLen, def}, then u32 returnTypeLen, returnType, u32 accessLen,
// access.
std::string udf_meta_serialize(const UdfMetaInfo &meta);
void udf_meta_deserialize(const char *data, UdfMetaInfo &meta);

// Runtime payload of a real UDF/closure Function value. The `fn` pointer is a
// JIT-compiled entry point with the webstrada UDF signature (see cf8.h). The
// `capturedScope` is the parent scope used for variable resolution inside the
// function; it is borrowed (owned by the request's temporary-variant cleanup),
// never freed by UDFInfo. Refcounted because Function values are copied around.
struct UDFInfo
{
    int refcount = 1;
    void *fn = nullptr;                    // JIT entry, cast to the UDF signature
    string name;                           // "" for anonymous closures
    bool isClosure = false;
    cfvariant *capturedScope = nullptr;    // borrowed
    std::vector<UdfParamInfo> params;      // declared parameters (introspection)
    string returnType;                     // "" -> "Any"
    string access;                         // e.g. "public"

    // Component-method handle: when componentMethodIndex >= 0 this Function
    // value is a reference to ComponentInfo::methods[componentMethodIndex] on
    // `component` (retained by the handle; see component.h). Invoking it
    // dispatches to the component method with the instance's scopes.
    int componentMethodIndex = -1;
    ComponentInstance *component = nullptr;
};

// Reference-counting helpers for UDFInfo (allocated by cfvariant_create_udf).
UDFInfo *udf_info_retain(UDFInfo *info);
void udf_info_release(UDFInfo *info);

}
