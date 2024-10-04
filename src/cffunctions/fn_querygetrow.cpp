/**
 * @file fn_querygetrow.cpp
 * @brief CFML querygetrow() built-in.
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

cfvariant *cf_querygetrow(const cfvariant *query, const cfvariant *row_number) {
    if (!query || query->m_type != cfvariant::Query || !query->m_query) {
        throw webstrada::exception("QueryGetRow: First argument must be a query");
    }
    if (!row_number) throw webstrada::exception("QueryGetRow requires a row number");
    QueryData *qd = query->m_query;
    int row = getIntValue(*row_number);
    int rc = qd->rowCount();
    if (row < 1 || row > rc) {
        throw webstrada::exception("QueryGetRow: Row number " + variantToString(*row_number) + " is out of bounds");
    }

    auto *ret = new cfvariant(cfvariant::Struct);
    for (auto &col : qd->columns) {
        ret->structSet(col.name, col.values[row - 1]);
    }
    return ret;
}

} // namespace cfml
