/**
 * @file fn_querykeyexists.cpp
 * @brief CFML querykeyexists() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <string>
#include <vector>

namespace cfml {

cfvariant *cf_querykeyexists(const cfvariant *query, const cfvariant *key) {
    if (!query || query->m_type != cfvariant::Query || !query->m_query) {
        throw webstrada::exception("QueryKeyExists: First argument must be a query");
    }
    bool exists = false;
    if (key) {
        string k = variantToString(*key);
        exists = query->m_query->findColumn(k) >= 0;
    }
    auto *ret = new cfvariant(cfvariant::Boolean);
    ret->m_bool = exists;
    return ret;
}

} // namespace cfml
