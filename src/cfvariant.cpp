#include <webstrada/cfvariant.h>
#include <webstrada/string.h>
#include <webstrada/cfimage.h>
#include <webstrada/component.h>
#include <webstrada/exceptions.h>

#include <stdexcept>
#include <utility>
#include <vector>
#include <set>
#include <unordered_set>
#include <unordered_map>
#include <string>
#include <map>
#include <cmath>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstdint>

#include <utility>

namespace webstrada {

// Decodes a raw byte buffer as UTF-8 with the replacement character (U+FFFD)
// for invalid sequences, exactly like Java's `new String(byte[])` — the
// default-charset decode ColdFusion applies to a cfhttp ByteArrayOutputStream.
// Every invalid byte yields one U+FFFD (3 UTF-8 bytes), so the returned byte
// string length can exceed the input length.
static string decodeUtf8ReplacementBytes(const std::vector<std::byte> &buf)
{
    string out;
    size_t i = 0;
    const size_t n = buf.size();
    while (i < n) {
        unsigned char c = static_cast<unsigned char>(buf[i]);
        if (c < 0x80) {
            out.append(static_cast<char>(c));
            i++;
        } else {
            size_t extra = 0;
            bool valid = false;
            if ((c & 0xE0) == 0xC0) extra = 1;
            else if ((c & 0xF0) == 0xE0) extra = 2;
            else if ((c & 0xF8) == 0xF0) extra = 3;
            if (extra > 0 && i + extra < n) {
                valid = true;
                for (size_t k = 1; k <= extra; k++) {
                    unsigned char cc = static_cast<unsigned char>(buf[i + k]);
                    if ((cc & 0xC0) != 0x80) { valid = false; break; }
                }
            }
            if (valid) {
                for (size_t k = 0; k <= extra; k++) out.append(static_cast<char>(buf[i + k]));
                i += extra + 1;
            } else {
                out.append("\xEF\xBF\xBD");   // U+FFFD replacement character
                i++;
            }
        }
    }
    return out;
}

} // namespace webstrada

#include <strings.h>
#include <unistd.h>


#define UNIMPLEMENTED std::runtime_error(std::string("Unimplemented. - ") + __PRETTY_FUNCTION__)
#define RUNTIME_WITH_STRING(text) std::runtime_error(std::string(text " - ") + __PRETTY_FUNCTION__)

#ifdef ENABLE_TRACE
  #define TRACE_FUNCTION() fprintf(stderr, "Start %p! %s: %d\n", this, __PRETTY_FUNCTION__, __LINE__)
  #define TRACE(...) fprintf(stderr, __VA_ARGS__)
#else
#define TRACE_FUNCTION()
#define TRACE(...)
#endif


using namespace webstrada;

UDFInfo *webstrada::udf_info_retain(UDFInfo *info)
{
    if (info) info->refcount++;
    return info;
}

void webstrada::udf_info_release(UDFInfo *info)
{
    if (info && --info->refcount <= 0) {
        if (info->component) component_instance_release(info->component);
        delete info;
    }
}

StructData *webstrada::struct_data_retain(StructData *sd)
{
    if (sd) sd->refs++;
    return sd;
}

namespace {

void collectStructChildren(const cfvariant &val, std::vector<StructData*> &out) {
    if (val.m_type == cfvariant::Struct || val.m_type == cfvariant::Xml ||
        val.m_type == cfvariant::JSon || val.m_type == cfvariant::Component) {
        if (val.m_structData) out.push_back(val.m_structData);
    } else if (val.m_type == cfvariant::Array && val.m_array) {
        for (const auto &elem : *val.m_array) {
            collectStructChildren(elem, out);
        }
    }
}

void getStructDataChildren(StructData *sd, std::vector<StructData*> &children) {
    for (const auto &pair : sd->map) {
        collectStructChildren(pair.second, children);
    }
}

thread_local bool in_cycle_collection = false;

void tryCollectCycles(StructData *root) {
    if (in_cycle_collection || !root || root->refs <= 0) return;
    in_cycle_collection = true;

    // 1. Gather reachable StructData nodes
    std::vector<StructData*> nodes;
    std::unordered_set<StructData*> visited;
    std::vector<StructData*> worklist = {root};
    visited.insert(root);

    while (!worklist.empty()) {
        StructData *curr = worklist.back();
        worklist.pop_back();
        nodes.push_back(curr);

        std::vector<StructData*> children;
        getStructDataChildren(curr, children);
        for (StructData *child : children) {
            if (child && visited.insert(child).second) {
                worklist.push_back(child);
            }
        }
    }

    // 2. Compute pending refcounts by subtracting internal references
    std::unordered_map<StructData*, int> pending;
    for (StructData *node : nodes) {
        pending[node] = node->refs;
    }

    for (StructData *node : nodes) {
        std::vector<StructData*> children;
        getStructDataChildren(node, children);
        for (StructData *child : children) {
            auto it = pending.find(child);
            if (it != pending.end()) {
                it->second--;
            }
        }
    }

    // 3. Mark nodes reachable from externally-referenced nodes (pending > 0) as alive
    std::unordered_set<StructData*> alive;
    std::vector<StructData*> aliveWorklist;
    for (StructData *node : nodes) {
        if (pending[node] > 0) {
            alive.insert(node);
            aliveWorklist.push_back(node);
        }
    }

    while (!aliveWorklist.empty()) {
        StructData *curr = aliveWorklist.back();
        aliveWorklist.pop_back();

        std::vector<StructData*> children;
        getStructDataChildren(curr, children);
        for (StructData *child : children) {
            if (child && alive.insert(child).second) {
                aliveWorklist.push_back(child);
            }
        }
    }

    // 4. Any node not in `alive` is dead (part of an unreferenced cycle).
    std::vector<StructData*> deadNodes;
    for (StructData *node : nodes) {
        if (alive.find(node) == alive.end()) {
            deadNodes.push_back(node);
        }
    }

    if (!deadNodes.empty()) {
        for (StructData *dead : deadNodes) {
            dead->refs++; // temporary protect
        }
        for (StructData *dead : deadNodes) {
            dead->map.clear();
            dead->insertOrder.clear();
        }
        for (StructData *dead : deadNodes) {
            dead->refs--; // unprotect
        }
        for (StructData *dead : deadNodes) {
            if (dead->refs <= 0) {
                delete dead->meta;
                dead->meta = nullptr;
                delete dead;
            }
        }
    }

    in_cycle_collection = false;
}

} // namespace

void webstrada::struct_data_release(StructData *sd)
{
    if (!sd) return;
    if (--sd->refs <= 0) {
        delete sd->meta;
        delete sd;
        return;
    }
    tryCollectCycles(sd);
}

QueryData *webstrada::query_data_retain(QueryData *qd)
{
    if (qd) qd->refs++;
    return qd;
}

void webstrada::query_data_release(QueryData *qd)
{
    if (!qd) return;
    if (--qd->refs > 0) return;
    delete qd;
}

namespace {

// Little-endian helpers for the UDF metadata blob (see udf_meta_serialize).
static void appendU32(std::string &out, uint32_t v)
{
    out.push_back(static_cast<char>(v & 0xff));
    out.push_back(static_cast<char>((v >> 8) & 0xff));
    out.push_back(static_cast<char>((v >> 16) & 0xff));
    out.push_back(static_cast<char>((v >> 24) & 0xff));
}

static uint32_t readU32(const char *&p, const char *end)
{
    uint32_t v = 0;
    if (p + 4 <= end) {
        v = static_cast<unsigned char>(p[0]) |
            (static_cast<unsigned char>(p[1]) << 8) |
            (static_cast<unsigned char>(p[2]) << 16) |
            (static_cast<unsigned char>(p[3]) << 24);
        p += 4;
    }
    return v;
}

static void readStringField(const char *&p, const char *end, string &out)
{
    uint32_t len = readU32(p, end);
    if (p + len <= end) {
        out = string(p, len);
        p += len;
    } else {
        p = end;
    }
}

}

std::string webstrada::udf_meta_serialize(const UdfMetaInfo &meta)
{
    std::string payload;
    appendU32(payload, static_cast<uint32_t>(meta.params.size()));
    for (const auto &param : meta.params) {
        const std::string name(param.name.constData(), param.name.length());
        const std::string type(param.type.constData(), param.type.length());
        const std::string def(param.defaultValue.constData(), param.defaultValue.length());
        appendU32(payload, static_cast<uint32_t>(name.size())); payload += name;
        appendU32(payload, static_cast<uint32_t>(type.size())); payload += type;
        appendU32(payload, static_cast<uint32_t>(def.size())); payload += def;
    }
    const std::string rt(meta.returnType.constData(), meta.returnType.length());
    const std::string acc(meta.access.constData(), meta.access.length());
    appendU32(payload, static_cast<uint32_t>(rt.size())); payload += rt;
    appendU32(payload, static_cast<uint32_t>(acc.size())); payload += acc;

    std::string out;
    appendU32(out, static_cast<uint32_t>(payload.size()));
    out += payload;
    return out;
}

void webstrada::udf_meta_deserialize(const char *data, UdfMetaInfo &meta)
{
    meta.params.clear();
    meta.returnType = "";
    meta.access = "";
    if (!data) return;

    const char *p = data;
    uint32_t totalLen = readU32(p, p + 4);
    const char *end = p + totalLen;
    if (end < p) end = p;  // overflow guard
    uint32_t paramCount = readU32(p, end);
    // Bound the param count defensively: a bogus blob must never loop forever.
    if (paramCount > 100000) paramCount = 0;
    meta.params.reserve(paramCount);
    for (uint32_t i = 0; i < paramCount; i++) {
        UdfParamInfo param;
        readStringField(p, end, param.name);
        readStringField(p, end, param.type);
        readStringField(p, end, param.defaultValue);
        meta.params.push_back(std::move(param));
    }
    readStringField(p, end, meta.returnType);
    readStringField(p, end, meta.access);
}

cfvariant::cfvariant(bool upcase, bool autocreate, bool readOnly, bool disabled)
    : m_upcase(upcase)
    , m_autocreate(autocreate)
    , m_readOnly(readOnly)
    , m_disabled(disabled)
{
}

cfvariant::cfvariant(int num)
{
    TRACE_FUNCTION();

    m_type = cfvariant::Number;
    m_int = num;
}

cfvariant::cfvariant(const char *str)
{
    TRACE_FUNCTION();

    m_type = cfvariant::String;
    m_str = new string(str);
    TRACE("m_str = new string(str); [%s] %p\n", m_str->constData(), m_str);
}

cfvariant::cfvariant(const string &str)
{
    TRACE_FUNCTION();

    m_type = cfvariant::String;
    m_str = new string(str);
    TRACE("m_str = new string(str); [%s] %p\n", m_str->constData(), m_str);
}

cfvariant::cfvariant(const cfvariantType type)
{
    TRACE_FUNCTION();

    set_type(type);
}

cfvariant::~cfvariant()
{
    TRACE_FUNCTION();

    set_type(NotSet);
}

cfvariant& cfvariant::operator=(const cfvariant& other)
{
    if (&other != this)
    {
        TRACE_FUNCTION();
        cfvariant temp(other);
        *this = std::move(temp);
    }

    return *this;
}

cfvariant::cfvariant(const cfvariant &other)
{
    m_type = other.m_type;
    m_upcase = other.m_upcase;
    m_autocreate = other.m_autocreate;
    m_readOnly = other.m_readOnly;
    m_disabled = other.m_disabled;
    m_boolLiteral = other.m_boolLiteral;
    m_isArguments = other.m_isArguments;
    m_argumentsParamCount = other.m_argumentsParamCount;
    m_serializeInsertOrder = other.m_serializeInsertOrder;
    m_isXmlNodeList = other.m_isXmlNodeList;
    m_isCustomException = other.m_isCustomException;
    m_isAbort = other.m_isAbort;
    m_isByteArrayOutputStream = other.m_isByteArrayOutputStream;
    m_odbcStyle = other.m_odbcStyle;
    m_tempRegistered = false;

    if (other.m_literalText) {
        m_literalText = new string(*other.m_literalText);
    }

    switch(m_type)
    {
    case NotSet:
    case Null:
        m_obj = nullptr;
        break;
    case Boolean:
        m_bool = other.m_bool;
        break;
    case Number:
        m_int = other.m_int;
        break;
    case Long:
        m_long = other.m_long;
        break;
    case Float:
        m_double = other.m_double;
        break;
    case DateTime:
        m_double = other.m_double;
        break;
    case String:
        m_str = new string(*other.m_str);
        break;
    case Function:
        m_str = new string(*other.m_str);
        m_udf = udf_info_retain(other.m_udf);
        break;
    case Array:
        m_array = new std::vector<cfvariant>(*other.m_array);
        break;
    case Struct:
    case Xml:
    case JSon:
    case Component:
        // Share the refcounted payload (CF reference semantics for structs).
        m_structData = struct_data_retain(other.m_structData);
        m_struct = m_structData ? &m_structData->map : nullptr;
        m_structInsertOrder = m_structData ? &m_structData->insertOrder : nullptr;
        if (m_type == Component) {
            m_component = component_instance_retain(other.m_component);
            m_superTargetInfo = other.m_superTargetInfo;
        }
        break;
    case Query:
        m_query = query_data_retain(other.m_query);
        break;
    case Image:
        m_image = image_data_retain(other.m_image);
        break;
    case Binary:
        m_binary = new std::vector<std::byte>(*other.m_binary);
        break;
    case File:
        throw RUNTIME_WITH_STRING("Cannot copy File type");
    default:
        UNIMPLEMENTED;
    }

    // The query-column back-reference is preserved by a copy (CF: a column
    // assigned to a variable keeps referencing its query). The QueryData is
    // retained so the column stays alive as long as any reference exists. A
    // copied column is writable only when it came from bracket access
    // (q["a"]); dot-access (q.a) copies throw on element write (see
    // cfvariant.h). m_queryColWritable starts at its default (true), so it is
    // set explicitly to the origin flag here.
    m_queryColOwner = query_data_retain(other.m_queryColOwner);
    m_queryColIndex = other.m_queryColIndex;
    m_queryColFromBracket = other.m_queryColFromBracket;
    m_queryColWritable = other.m_queryColFromBracket;
    m_queryColCopyDepth = other.m_queryColCopyDepth;
}

cfvariant::cfvariant(cfvariant&& other) noexcept
{
    TRACE_FUNCTION();

    std::swap(m_type, other.m_type);
    std::swap(m_obj, other.m_obj);
    std::swap(m_udf, other.m_udf);
    std::swap(m_literalText, other.m_literalText);
    std::swap(m_structData, other.m_structData);
    std::swap(m_structInsertOrder, other.m_structInsertOrder);
    std::swap(m_query, other.m_query);
    std::swap(m_image, other.m_image);
    std::swap(m_component, other.m_component);
    std::swap(m_superTargetInfo, other.m_superTargetInfo);
    std::swap(m_queryColOwner, other.m_queryColOwner);
    std::swap(m_queryColIndex, other.m_queryColIndex);
    std::swap(m_queryColFromBracket, other.m_queryColFromBracket);
    std::swap(m_queryColWritable, other.m_queryColWritable);
    std::swap(m_queryColCopyDepth, other.m_queryColCopyDepth);
    std::swap(m_boolLiteral, other.m_boolLiteral);
    std::swap(m_isArguments, other.m_isArguments);
    std::swap(m_argumentsParamCount, other.m_argumentsParamCount);
    std::swap(m_serializeInsertOrder, other.m_serializeInsertOrder);
    std::swap(m_isXmlNodeList, other.m_isXmlNodeList);
    std::swap(m_isCustomException, other.m_isCustomException);
    std::swap(m_isAbort, other.m_isAbort);
    std::swap(m_isByteArrayOutputStream, other.m_isByteArrayOutputStream);
    std::swap(m_odbcStyle, other.m_odbcStyle);
    m_tempRegistered = false;
}

cfvariant& cfvariant::operator=(cfvariant&& other) noexcept
{
    TRACE_FUNCTION();

    std::swap(m_type, other.m_type);
    std::swap(m_obj, other.m_obj);
    std::swap(m_udf, other.m_udf);
    std::swap(m_literalText, other.m_literalText);
    std::swap(m_structData, other.m_structData);
    std::swap(m_structInsertOrder, other.m_structInsertOrder);
    std::swap(m_query, other.m_query);
    std::swap(m_image, other.m_image);
    std::swap(m_component, other.m_component);
    std::swap(m_superTargetInfo, other.m_superTargetInfo);
    std::swap(m_queryColOwner, other.m_queryColOwner);
    std::swap(m_queryColIndex, other.m_queryColIndex);
    std::swap(m_queryColFromBracket, other.m_queryColFromBracket);
    std::swap(m_queryColWritable, other.m_queryColWritable);
    std::swap(m_queryColCopyDepth, other.m_queryColCopyDepth);
    std::swap(m_boolLiteral, other.m_boolLiteral);
    std::swap(m_isArguments, other.m_isArguments);
    std::swap(m_argumentsParamCount, other.m_argumentsParamCount);
    std::swap(m_serializeInsertOrder, other.m_serializeInsertOrder);
    std::swap(m_isXmlNodeList, other.m_isXmlNodeList);
    std::swap(m_isCustomException, other.m_isCustomException);
    std::swap(m_isAbort, other.m_isAbort);
    std::swap(m_isByteArrayOutputStream, other.m_isByteArrayOutputStream);
    std::swap(m_odbcStyle, other.m_odbcStyle);
    m_tempRegistered = false;

    return *this;
}

cfvariant &cfvariant::operator[](const int index) const
{
    TRACE_FUNCTION();

    if (m_type != cfvariantType::Array) {
        throw RUNTIME_WITH_STRING("Variable is not array.");
    }

    return m_array->at(index);
}

cfvariant &cfvariant::operator[](const char *key)
{
    TRACE_FUNCTION();

    if (m_type == cfvariantType::NotSet) {
        set_type(cfvariant::Struct);
    }

    if (m_type != cfvariantType::Struct && m_type != cfvariantType::Xml && m_type != cfvariantType::Component) {
        throw RUNTIME_WITH_STRING("Variable is not struct or xml.");
    }

    if (m_autocreate) {
        if (!m_struct->contains(key)) {
            auto newVal = cfvariant(m_upcase, m_autocreate, m_readOnly, m_disabled);
            m_struct->insert_or_assign(key, newVal);
            if (m_structInsertOrder) m_structInsertOrder->push_back(key);
        }
    }

    return m_struct->at(key);
}

cfvariant &cfvariant::operator[](const string key)
{
    TRACE_FUNCTION();

    if (m_type == cfvariantType::NotSet) {
        set_type(cfvariant::Struct);
    }

    if (m_type != cfvariantType::Struct && m_type != cfvariantType::Xml && m_type != cfvariantType::Component) {
        throw RUNTIME_WITH_STRING("Variable is not struct or xml.");
    }

    if (m_autocreate) {
        if (!m_struct->contains(key)) {
            auto newVal = cfvariant(m_upcase, m_autocreate, m_readOnly, m_disabled);
            m_struct->insert_or_assign(key, newVal);
            if (m_structInsertOrder) m_structInsertOrder->push_back(key);
        }
    }

    auto tmp = m_struct->at(key);

    return m_struct->at(key);
}

cfvariant &cfvariant::operator[](const cfvariant &key) const
{
    TRACE_FUNCTION();

    if (m_type == cfvariantType::Array) {
        int index;
        switch (key.m_type) {
        case cfvariantType::Number: index = key.m_int; break;
        case cfvariantType::Long: index = static_cast<int>(key.m_long); break;
        case cfvariantType::Float: index = static_cast<int>(key.m_double); break;
        case cfvariantType::Boolean: index = key.m_bool ? 1 : 0; break;
        default: {
            // Mirror the runtime getIntValue coercion: a non-numeric key is
            // parsed as an integer string (e.g. arr[cfvariant("2")] -> index 2).
            const string s = const_cast<cfvariant&>(key).toString();
            const char *str = s.constData();
            if (!str) throw RUNTIME_WITH_STRING("Expected a numeric value but received empty/null.");
            const char *p = str;
            while (*p && isspace(*p)) p++;
            if (*p == '\0') throw RUNTIME_WITH_STRING("Expected a numeric value but received an empty string.");
            char *end = nullptr;
            long val = strtol(p, &end, 10);
            while (end && *end && isspace(*end)) end++;
            if (end == p || (end && *end != '\0'))
                throw std::runtime_error(std::string("Parameter validation error: The value '") + str + "' cannot be converted to an integer.");
            index = static_cast<int>(val);
            break;
        }
        }
        if (index < 1 || index > static_cast<int>(m_array->size())) {
            throw RUNTIME_WITH_STRING("Array index out of bounds");
        }
        return m_array->at(index - 1);
    }

    if (m_type == cfvariantType::Struct || m_type == cfvariantType::Xml || m_type == cfvariantType::Component) {
        // Const-qualified struct lookup: never autocreates (unlike the non-const
        // overloads); a missing key throws, matching the siblings' m_struct->at().
        string k = const_cast<cfvariant&>(key).toString();
        return m_struct->at(k);
    }

    throw RUNTIME_WITH_STRING("Variable is not struct or xml.");
}

bool cfvariant::operator==(const string &other) const
{
    if (m_type != String)
        return false;

    return m_str->equals(other);
}

bool cfvariant::operator==(const char *other) const
{
    if (m_type != String)
        return false;

    return m_str->equals(other);
}

cfvariant &cfvariant::set(const int index)
{
    TRACE_FUNCTION();

    throw UNIMPLEMENTED;
}

cfvariant &cfvariant::set(const char *key)
{
    TRACE_FUNCTION();

    if (m_type != cfvariantType::Struct && m_type != cfvariantType::Xml && m_type != cfvariantType::Component) {
        throw RUNTIME_WITH_STRING("Variable is not struct or xml.");
    }

    // Keys keep the casing they were first inserted with; the case-insensitive
    // map comparator makes lookups case-insensitive regardless.
    string k(key);
    if (m_struct->find(k) == m_struct->end()) {
        m_struct->insert_or_assign(k, cfvariant(m_upcase, m_autocreate, m_readOnly, m_disabled));
        if (m_structInsertOrder) m_structInsertOrder->push_back(k);
    }

    return m_struct->at(k);
}

cfvariant &cfvariant::set(const string &key)
{
    TRACE_FUNCTION();

    if (m_type != cfvariantType::Struct && m_type != cfvariantType::Xml && m_type != cfvariantType::Component) {
        throw RUNTIME_WITH_STRING("Variable is not struct or xml.");
    }

    // See set(const char*): preserve original key casing, case-insensitive lookup.
    if (m_struct->find(key) == m_struct->end()) {
        m_struct->insert_or_assign(key, cfvariant(m_upcase, m_autocreate, m_readOnly, m_disabled));
        if (m_structInsertOrder) m_structInsertOrder->push_back(key);
    }

    return m_struct->at(key);
}

cfvariant &cfvariant::structSet(const string &key, const cfvariant &value)
{
    TRACE_FUNCTION();

    if (m_type != cfvariantType::Struct && m_type != cfvariantType::Xml && m_type != cfvariantType::Component) {
        throw RUNTIME_WITH_STRING("Variable is not struct or xml.");
    }

    auto res = m_struct->insert_or_assign(key, value);
    if (res.second && m_structInsertOrder) {
        m_structInsertOrder->push_back(key);
    }
    return res.first->second;
}

cfvariant &cfvariant::set(const cfvariant &key)
{
    const char *str_key = nullptr;

    TRACE_FUNCTION();

    if (key.m_type != String) {
        throw RUNTIME_WITH_STRING("Variable is not string.");
    }

    str_key = key.m_str->constData();

    return set(str_key);
}

void cfvariant::set_type(const cfvariantType type)
{
    TRACE_FUNCTION();

    if (type == m_type)
        return;

    // The query-column back-reference is transient (only meaningful on a temp
    // column array while it is an assignment target); it never survives a type
    // change. Release the retained QueryData so a column reference that dies
    // (or is overwritten) does not leak it.
    if (m_queryColOwner) {
        query_data_release(m_queryColOwner);
        m_queryColOwner = nullptr;
    }
    m_queryColIndex = -1;
    m_queryColFromBracket = false;
    m_queryColWritable = true;
    m_queryColCopyDepth = 0;

    // Free the float literal text sidecar when the value changes type.
    if (m_literalText != nullptr) {
        delete m_literalText;
        m_literalText = nullptr;
    }

    // delete old type.
    switch(m_type)
    {
    case String:
        if (m_str != nullptr) {
            delete m_str;
            m_str = nullptr;
        }
        break;
    case Function:
        if (m_str != nullptr) {
            delete m_str;
            m_str = nullptr;
        }
        udf_info_release(m_udf);
        m_udf = nullptr;
        break;
    case Array:
        if (m_array != nullptr) {
            delete m_array;
            m_array = nullptr;
        }
        break;
    case Struct:
    case Xml:
    case JSon:
    case Component:
        struct_data_release(m_structData);
        m_structData = nullptr;
        m_struct = nullptr;
        m_structInsertOrder = nullptr;
        if (m_type == Component) {
            component_instance_release(m_component);
            m_component = nullptr;
        }
        break;
    case Image:
        image_data_release(m_image);
        m_image = nullptr;
        break;
    case Query:
        query_data_release(m_query);
        m_query = nullptr;
        break;
    case Binary:
        if (m_binary != nullptr) {
            delete m_binary;
            m_binary = nullptr;
        }
        break;
    case File:
        if (m_fd > 2)
            close(m_fd);
        m_fd = 0;
        break;
    default:
        break;
    }

    switch(type)
    {
    case NotSet:
    case Null:
        m_obj = nullptr;
        break;
    case Boolean:
        m_bool = false;
        break;
    case Number:
        m_int = 0;
        break;
    case Long:
        m_long = 0;
        break;
    case Float:
        m_double = 0.0;
        break;
    case String:
    case Function:
        m_str = new string();
        break;
    case DateTime:
        m_double = 0;
        break;
    case Array:
        m_array = new std::vector<cfvariant>();
        break;
    case Struct:
    case Xml:
    case JSon:
    case Component:
        m_structData = new StructData();
        m_struct = &m_structData->map;
        m_structInsertOrder = &m_structData->insertOrder;
        if (type == Component) m_component = nullptr;
        break;
    case Image:
        m_image = nullptr;
        break;
    case Query:
        m_query = new QueryData();
        break;
    case Binary:
        m_binary = new std::vector<std::byte>();
        break;
    case File:
        m_fd = -1;
        break;
    }

    m_type = type;
    if (type != Struct && type != Xml) {
        m_isArguments = false;
        m_argumentsParamCount = 0;
        m_isCustomException = false;
        m_isAbort = false;
        m_isByteArrayOutputStream = false;
    }
}

void cfvariant::setUpcase(bool upcase)
{
    TRACE_FUNCTION();

    m_upcase = upcase;
}

void cfvariant::setAutoCreate()
{
    TRACE_FUNCTION();

    m_autocreate = true;
}

void cfvariant::setReadOnly()
{
    TRACE_FUNCTION();

    m_readOnly = true;
}

void cfvariant::setDisabled()
{
    TRACE_FUNCTION();

    m_disabled = true;
}

void cfvariant::insert(const cfvariant &item)
{
    if (m_type != Array) {
        throw UNIMPLEMENTED;
    }

    m_array->push_back(item);
}

string cfvariant::join(char separator) const
{
    string ret;

    if(m_type != Array) {
        throw UNIMPLEMENTED;
    }

    int size = m_array->size();

    if (size > 0) {
        ret += m_array->at(0);

        for(int c = 1; c < size; c++) {
            ret += separator;
            ret += m_array->at(c);
        }
    }

    return ret;
}

bool cfvariant::has(const char *key) const
{
    if (m_type != Struct && m_type != Xml)
        return false;

    return m_struct->contains(key);
}

// CF renders floats with %.12g and a normalized 3-digit exponent
// (1/100000 → 1E-005, 2^53 → 9.00719925474E+015); Infinity renders as
// 1.#INF / -1.#INF and NaN as the U+FFFD replacement character, matching
// CF 2021 (which stringifies a NaN double to a single U+FFFD character).
static string formatFloatString(double d)
{
    if (std::isinf(d)) return d > 0 ? string("1.#INF") : string("-1.#INF");
    if (std::isnan(d)) return string("\xEF\xBF\xBD");
    if (d == 0.0) return string("0"); // CF renders computed -0.0 as "0"
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%.12g", d);
    std::string s(buf);
    size_t e = s.find_first_of("eE");
    if (e != std::string::npos) {
        std::string mant = s.substr(0, e);
        int exp = std::atoi(s.c_str() + e + 1);
        char eb[32];
        std::snprintf(eb, sizeof(eb), "%+04d", exp);
        s = mant + "E" + eb;
    }
    return string(s.c_str());
}

const string cfvariant::toString()
{
    if (m_type == String)
        return *m_str;

    // A query-column reference (q.a / q["a"] materialization) stringifies as
    // its scalar cell at the query's current cursor position (CF: `x = q["a"];`
    // prints the current row's cell; inside a <cfloop query> that is the row
    // being iterated). The query stays alive via the retained reference.
    if (m_type == Array && m_queryColOwner && m_queryColIndex >= 0) {
        QueryData *qd = m_queryColOwner;
        int colIdx = m_queryColIndex;
        if (colIdx >= 0 && colIdx < (int)qd->columns.size() &&
            !qd->columns[colIdx].values.empty()) {
            int row = qd->currentRow;
            if (row < 1) row = 1;
            if (row <= (int)qd->columns[colIdx].values.size()) {
                return qd->columns[colIdx].values[row - 1].toString();
            }
        }
        return "";
    }

    // A Function value is a method handle for a built-in CFML function
    // (coldfusion.runtime.CFPageMethod@<hash>); its display text is stored in
    // the string slot like a String.
    if (m_type == Function)
        return *m_str;

    if (m_type == DateTime) {
        double days = m_double;
        double int_part;
        double frac_part = std::modf(days, &int_part);
        if (frac_part < 0.0) {
            frac_part += 1.0;
            int_part -= 1.0;
        }

        long jdn = static_cast<long>(int_part) + 2415019;

        long l = jdn + 68569;
        long n = (4 * l) / 146097;
        l = l - (146097 * n + 3) / 4;
        long i = (4000 * (l + 1)) / 1461001;
        l = l - (1461 * i) / 4 + 31;
        long j = (80 * l) / 2447;
        int mday = l - (2447 * j) / 80;
        l = j / 11;
        int mon = j + 2 - 12 * l;
        int year = 100 * (n - 49) + i + l;

        double total_seconds = frac_part * 86400.0 + 0.5; // with rounding
        int hour = static_cast<int>(total_seconds) / 3600;
        int min = (static_cast<int>(total_seconds) % 3600) / 60;
        int sec = static_cast<int>(total_seconds) % 60;

        char buf[64];
        if (m_odbcStyle == 1) {
            std::snprintf(buf, sizeof(buf), "{d '%04d-%02d-%02d'}", year, mon, mday);
            return string(buf);
        }
        if (m_odbcStyle == 2) {
            std::snprintf(buf, sizeof(buf), "{t '%02d:%02d:%02d'}", hour, min, sec);
            return string(buf);
        }
        if (m_odbcStyle == 3) {
            // CF's QueryTable renders java.util.Date cells (cfdirectory's
            // dateLastModified) with the default locale mask
            // MM/dd/yyyy hh:nn:ss tt (12-hour with AM/PM).
            int h12 = hour % 12;
            if (h12 == 0) h12 = 12;
            std::snprintf(buf, sizeof(buf), "%02d/%02d/%04d %02d:%02d:%02d %s",
                          mon, mday, year, h12, min, sec, hour < 12 ? "AM" : "PM");
            return string(buf);
        }
        std::snprintf(buf, sizeof(buf), "{ts '%04d-%02d-%02d %02d:%02d:%02d'}", year, mon, mday, hour, min, sec);
        return string(buf);
    }

    if (m_type == Boolean)
        // CF stringifies literal booleans as true/false and computed booleans
        // (comparisons, not, boolean-returning functions) as YES/NO.
        return m_boolLiteral ? (m_bool ? string("true") : string("false"))
                             : (m_bool ? string("YES") : string("NO"));

    if (m_type == Number)
        return string::number(m_int);

    if (m_type == Long)
        return string::number(m_long);

    if (m_type == Float)
        return m_literalText ? *m_literalText : formatFloatString(m_double);

    if (m_type == Binary) {
        if (m_isByteArrayOutputStream) {
            // ColdFusion's ByteArrayOutputStream (cfhttp getasbinary="no" with a
            // non-text MIME body) stringifies as the bytes decoded with the
            // default charset (UTF-8, invalid bytes -> U+FFFD).
            const std::vector<std::byte> *bytes = m_binary;
            if (bytes && !bytes->empty()) {
                return decodeUtf8ReplacementBytes(*bytes);
            }
            return "";
        }
        throw webstrada::exception("Cannot convert data of type Binary to a string");
    }

    return "";
}

// Recursively independent copy (StructCopy / Duplicate). Ordinary copy and
// assignment share Struct/Query payloads (CF reference semantics); this
// clones every nested struct/array/query so the result shares nothing.
// `visited` guards against self-referential values (`s = variables`) that are
// now reachable through aliasing; a cycle throws instead of recursing forever.
static cfvariant deepCopyImpl(const cfvariant &src, std::set<const void*> &visited)
{
    const cfvariant &self = src;
    cfvariant out(self.m_type);
    out.m_upcase = self.m_upcase;
    out.m_autocreate = self.m_autocreate;
    out.m_readOnly = self.m_readOnly;
    out.m_disabled = self.m_disabled;
    out.m_boolLiteral = self.m_boolLiteral;
    out.m_isArguments = self.m_isArguments;
    out.m_argumentsParamCount = self.m_argumentsParamCount;
    out.m_serializeInsertOrder = self.m_serializeInsertOrder;
    out.m_isXmlNodeList = self.m_isXmlNodeList;
    out.m_isCustomException = self.m_isCustomException;
    out.m_isAbort = self.m_isAbort;
    out.m_isByteArrayOutputStream = self.m_isByteArrayOutputStream;
    if (self.m_literalText) out.m_literalText = new string(*self.m_literalText);

    switch (self.m_type)
    {
    case cfvariant::NotSet:
    case cfvariant::Null:
        break;
    case cfvariant::Boolean:
        out.m_bool = self.m_bool;
        break;
    case cfvariant::Number:
        out.m_int = self.m_int;
        break;
    case cfvariant::Long:
        out.m_long = self.m_long;
        break;
    case cfvariant::Float:
    case cfvariant::DateTime:
        out.m_double = self.m_double;
        break;
    case cfvariant::String:
        *out.m_str = *self.m_str;
        break;
    case cfvariant::Function:
        *out.m_str = *self.m_str;
        out.m_udf = udf_info_retain(self.m_udf);
        break;
    case cfvariant::Array: {
        const void *akey = self.m_array;
        if (akey && !visited.insert(akey).second) {
            throw webstrada::exception("Cannot copy a circular reference.");
        }
        out.m_array->reserve(self.m_array->size());
        for (const auto &el : *self.m_array) out.m_array->push_back(deepCopyImpl(el, visited));
        if (akey) visited.erase(akey);
        break;
    }
    case cfvariant::Struct:
    case cfvariant::Xml:
    case cfvariant::JSon:
    case cfvariant::Component: {
        const void *skey = self.m_structData;
        if (skey && !visited.insert(skey).second) {
            throw webstrada::exception("Cannot copy a circular reference.");
        }
        for (const auto &kv : *self.m_struct) {
            out.m_struct->insert_or_assign(kv.first, deepCopyImpl(kv.second, visited));
        }
        if (self.m_structInsertOrder) *out.m_structInsertOrder = *self.m_structInsertOrder;
        if (self.m_structData && self.m_structData->meta) out.m_structData->meta = new cfvariant(deepCopyImpl(*self.m_structData->meta, visited));
        if (self.m_type == cfvariant::Component) {
            out.m_component = component_instance_retain(self.m_component);
        }
        if (skey) visited.erase(skey);
        break;
    }
    case cfvariant::Query: {
        const void *qkey = self.m_query;
        if (qkey && !visited.insert(qkey).second) {
            throw webstrada::exception("Cannot copy a circular reference.");
        }
        out.m_query->columns = self.m_query->columns;
        out.m_query->currentRow = self.m_query->currentRow;
        out.m_query->m_rowCount = self.m_query->m_rowCount;
        for (auto &col : out.m_query->columns) {
            for (auto &cell : col.values) cell = deepCopyImpl(cell, visited);
        }
        if (qkey) visited.erase(qkey);
        break;
    }
    case cfvariant::Image:
        out.m_image = image_data_retain(self.m_image);
        break;
    case cfvariant::Binary:
        *out.m_binary = *self.m_binary;
        break;
    case cfvariant::File:
        throw RUNTIME_WITH_STRING("Cannot copy cfvariant::File type");
    default:
        throw RUNTIME_WITH_STRING("Unsupported type in deepCopy.");
    }
    return out;
}

cfvariant cfvariant::deepCopy() const
{
    std::set<const void*> visited;
    return deepCopyImpl(*this, visited);
}
