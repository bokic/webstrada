/**
 * @file fn_emit_writedump.cpp
 * @brief CFML emit_writedump() built-in.
 */

#include "common.h"

#include "../cftags/common.h"
#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>
#include <webstrada/exceptions.h>
#include <webstrada/string.h>
#include <algorithm>
#include <cmath>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <set>
#include <string>
#include <vector>

using webstrada::cfvariant;
using webstrada::string;
using webstrada::UdfParamInfo;

namespace cfml {

void cf_emit_writedump(void *out, cfvariant *dumpResult)
{
    bool abort = g_cfdump_abort_pending;
    g_cfdump_abort_pending = false;
    if (cfml::response().binary) { // cfcontent file/variable: other output ignored
        if (abort) throw webstrada::abort_exception();
        return;
    }
    if (out && dumpResult && dumpResult->m_type == cfvariant::String && dumpResult->m_str) {
        string *outStr = static_cast<string*>(out);
        const string &s = *dumpResult->m_str;
        if (s.startWith("<style>") && !outStr->isEmpty() && outStr->at(outStr->length() - 1) != ' ') {
            outStr->append(' ');
        }
        outStr->append(s);
    }
    if (abort) {
        throw webstrada::abort_exception();
    }
}

} // namespace cfml
