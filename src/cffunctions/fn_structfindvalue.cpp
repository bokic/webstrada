/**
 * @file fn_structfindvalue.cpp
 * @brief CFML structfindvalue() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <algorithm>
#include <set>
#include <vector>

namespace cfml {

static bool variantsEqual(const cfvariant &a, const cfvariant &b)
{
    bool aNum = a.m_type == cfvariant::Number || a.m_type == cfvariant::Long ||
                a.m_type == cfvariant::Float || a.m_type == cfvariant::DateTime;
    bool bNum = b.m_type == cfvariant::Number || b.m_type == cfvariant::Long ||
                b.m_type == cfvariant::Float || b.m_type == cfvariant::DateTime;
    if (aNum && bNum) return getDoubleValue(a) == getDoubleValue(b);
    if (a.m_type == cfvariant::Null || b.m_type == cfvariant::Null) return a.m_type == b.m_type;
    string sa = variantToString(a);
    string sb = variantToString(b);
    return sa.compareCaseInsensitive(sb) == 0;
}

static void structFindValueRecursive(const cfvariant &node, const cfvariant &searchValue, bool all,
                                     const string &path, std::vector<cfvariant> &results,
                                     std::set<const void*> &visited)
{
    if (node.m_type == cfvariant::Struct && node.m_struct) {
        const void *skey = node.m_structData;
        if (skey && !visited.insert(skey).second) return;
        for (const auto &p : *node.m_struct) {
            if (variantsEqual(p.second, searchValue)) {
                cfvariant res(cfvariant::Struct);
                res.structSet("KEY", p.first);
                res.structSet("PATH", cfvariant(path + "." + p.first));
                cfvariant owner(node);
                res.structSet("OWNER", owner);
                results.push_back(res);
                if (!all) { if (skey) visited.erase(skey); return; }
            }
        }
        for (const auto &p : *node.m_struct) {
            string childPath = path + "." + p.first;
            structFindValueRecursive(p.second, searchValue, all, childPath, results, visited);
            if (!all && !results.empty()) break;
        }
        if (skey) visited.erase(skey);
    } else if (node.m_type == cfvariant::Array && node.m_array) {
        const void *akey = node.m_array;
        if (akey && !visited.insert(akey).second) return;
        for (const auto &item : *node.m_array) {
            structFindValueRecursive(item, searchValue, all, path, results, visited);
            if (!all && !results.empty()) break;
        }
        if (akey) visited.erase(akey);
    }
}

cfvariant *cf_structfindvalue(const cfvariant *top, const cfvariant *value, const cfvariant *scope) {
    if (!top || !value) throw webstrada::exception("StructFindValue requires top and value arguments");
    bool all = false;
    if (scope && scope->m_type != cfvariant::Null) {
        string sc = const_cast<cfvariant*>(scope)->toString();
        all = sc.compareCaseInsensitive("all") == 0;
    }
    std::vector<cfvariant> results;
    std::set<const void*> visited;
    structFindValueRecursive(*top, *value, all, "", results, visited);
    auto *ret = new cfvariant(cfvariant::Array);
    for (auto &r : results) ret->insert(r);
    return ret;
}

} // namespace cfml
