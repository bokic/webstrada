/**
 * @file fn_querygetresult.cpp
 * @brief CFML querygetresult() built-in.
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

cfvariant *cf_querygetresult(const cfvariant *query) {
    // CF's QueryGetResult returns the cached <cfquery> resultset metadata; a
    // QueryNew-built query has none, so CF returns null (verified vs CF 2021:
    // cfdump of QueryGetResult(q) renders "undefined"). All queries in this
    // engine come from QueryNew, so null is the faithful result.
    if (!query || query->m_type != cfvariant::Query || !query->m_query) {
        throw webstrada::exception("QueryGetResult: First argument must be a query");
    }
    auto *ret = new cfvariant(cfvariant::Null);
    return ret;
}

} // namespace cfml
