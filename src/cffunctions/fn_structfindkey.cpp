/**
 * @file fn_structfindkey.cpp
 * @brief CFML structfindkey() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <algorithm>
#include <set>
#include <vector>

namespace cfml {

static void structFindKeyRecursive(const cfvariant &node, const string &searchKey, bool all,
                                   const string &path, std::vector<cfvariant> &results,
                                   std::set<const void*> &visited)
{
    if (node.m_type == cfvariant::Struct && node.m_struct) {
        const void *skey = node.m_structData;
        if (skey && !visited.insert(skey).second) return;
        for (const auto &p : *node.m_struct) {
            if (p.first.compareCaseInsensitive(searchKey) == 0) {
                cfvariant res(cfvariant::Struct);
                res.structSet("PATH", cfvariant(path + "." + searchKey));
                cfvariant owner(node);
                res.structSet("OWNER", owner);
                res.structSet("VALUE", p.second);
                results.push_back(res);
                if (!all) { if (skey) visited.erase(skey); return; }
            }
        }
        for (const auto &p : *node.m_struct) {
            string childPath = path + "." + p.first;
            structFindKeyRecursive(p.second, searchKey, all, childPath, results, visited);
            if (!all && !results.empty()) break;
        }
        if (skey) visited.erase(skey);
    } else if (node.m_type == cfvariant::Array && node.m_array) {
        const void *akey = node.m_array;
        if (akey && !visited.insert(akey).second) return;
        for (const auto &item : *node.m_array) {
            structFindKeyRecursive(item, searchKey, all, path, results, visited);
            if (!all && !results.empty()) break;
        }
        if (akey) visited.erase(akey);
    }
}

cfvariant *cf_structfindkey(const cfvariant *top, const cfvariant *value, const cfvariant *scope) {
    if (!top || !value) throw webstrada::exception("StructFindKey requires top and value arguments");
    string searchKey = const_cast<cfvariant*>(value)->toString();
    bool all = false;
    if (scope && scope->m_type != cfvariant::Null) {
        string sc = const_cast<cfvariant*>(scope)->toString();
        all = sc.compareCaseInsensitive("all") == 0;
    }
    std::vector<cfvariant> results;
    std::set<const void*> visited;
    structFindKeyRecursive(*top, searchKey, all, "", results, visited);
    auto *ret = new cfvariant(cfvariant::Array);
    for (auto &r : results) ret->insert(r);
    return ret;
}

} // namespace cfml
