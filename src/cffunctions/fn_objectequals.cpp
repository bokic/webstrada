/**
 * @file fn_objectequals.cpp
 * @brief CFML objectequals() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>
#include <webstrada/exceptions.h>
#include <webstrada/string.h>
#include <map>
#include <string>

namespace cfml {

// Deep, type-strict equality following ColdFusion's ObjectEquals
// (CFComparable.objectEquals): values of different CF types are never equal
// (ObjectEquals(1, "1") and ObjectEquals(1, 1.0) are both NO on CF 2025),
// containers are compared recursively, and identity wins for reference types.
static bool objectEqualsRec(const cfvariant &a, const cfvariant &b,
                            std::map<std::pair<const void*, const void*>, bool> &seen) {
    if (&a == &b) return true;
    if (a.m_type != b.m_type) return false;

    switch (a.m_type) {
    case cfvariant::NotSet:
    case cfvariant::Null:
        return true;
    case cfvariant::Boolean:
        return a.m_bool == b.m_bool;
    case cfvariant::Number:
        return a.m_int == b.m_int;
    case cfvariant::Long:
        return a.m_long == b.m_long;
    case cfvariant::Float:
        return a.m_double == b.m_double;
    case cfvariant::String: {
        string sa = const_cast<cfvariant&>(a).toString();
        string sb = const_cast<cfvariant&>(b).toString();
        return sa.equals(sb);
    }
    case cfvariant::DateTime:
        return a.m_double == b.m_double;
    case cfvariant::Array: {
        if (a.m_array->size() != b.m_array->size()) return false;
        for (size_t i = 0; i < a.m_array->size(); i++) {
            if (!objectEqualsRec(a.m_array->at(i), b.m_array->at(i), seen)) return false;
        }
        return true;
    }
    case cfvariant::Struct:
    case cfvariant::Xml: {
        if (a.m_struct->size() != b.m_struct->size()) return false;
        for (const auto &kv : *a.m_struct) {
            auto it = b.m_struct->find(kv.first);
            if (it == b.m_struct->end()) return false;
            if (!objectEqualsRec(kv.second, it->second, seen)) return false;
        }
        return true;
    }
    case cfvariant::Binary: {
        if (!a.m_binary || !b.m_binary) return a.m_binary == b.m_binary;
        return *a.m_binary == *b.m_binary;
    }
    case cfvariant::Query: {
        if (!a.m_query || !b.m_query) return a.m_query == b.m_query;
        if (a.m_query->rowCount() != b.m_query->rowCount()) return false;
        if (a.m_query->columns.size() != b.m_query->columns.size()) return false;
        for (size_t c = 0; c < a.m_query->columns.size(); c++) {
            if (a.m_query->columns[c].name.compareCaseInsensitive(b.m_query->columns[c].name.constData()) != 0)
                return false;
            if (a.m_query->columns[c].values.size() != b.m_query->columns[c].values.size())
                return false;
            for (size_t r = 0; r < a.m_query->columns[c].values.size(); r++) {
                if (!objectEqualsRec(a.m_query->columns[c].values[r], b.m_query->columns[c].values[r], seen))
                    return false;
            }
        }
        return true;
    }
    default:
        // Function/Component/Image/File/JSon: reference identity (CF objects
        // are reference types).
        return a.m_obj == b.m_obj;
    }
}

cfvariant *cf_objectequals(const cfvariant *clientobject, const cfvariant *originalobject) {
    if (!clientobject || !originalobject)
        throw webstrada::exception("ObjectEquals requires exactly 2 arguments");
    std::map<std::pair<const void*, const void*>, bool> seen;
    bool res = objectEqualsRec(*clientobject, *originalobject, seen);
    auto *ret = new cfvariant(cfvariant::Boolean);
    ret->m_bool = res;
    return ret;
}

} // namespace cfml
