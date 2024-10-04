/**
 * @file fn_getmetricdata.cpp
 * @brief CFML getmetricdata() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>
#include <webstrada/exceptions.h>
#include <webstrada/string.h>
#include <string>

namespace cfml {

cfvariant *cf_getmetricdata(const cfvariant *mode) {
    if (!mode) throw webstrada::exception("GetMetricData requires exactly 1 argument");
    string modeStr = const_cast<cfvariant*>(mode)->toString();
    string m = modeStr;
    m.toUpper();

    if (m.equals("SIMPLE_LOAD") || m.equals("PREV_REQ_TIME") || m.equals("AVG_REQ_TIME")) {
        // Server-state dependent; 0 mirrors an idle server (the RDS host
        // reports small live values). Cannot be byte-verified against CF.
        auto *ret = new cfvariant(cfvariant::Long);
        ret->m_long = 0;
        return ret;
    }

    if (m.equals("PERF_MONITOR")) {
        // CF's perf_monitor struct key set (verified against CF 2025 on the
        // RDS host); the values are live server counters, so this engine
        // reports the idle zeros. See BUGS_COSMETIC.md.
        cfvariant st(cfvariant::Struct);
        const char *keys[] = {
            "reqqueued", "cfcreqqueued", "dbhits", "wsreqqueued", "avgreqtime",
            "bytesout", "templatereqtimedout", "wsreqrunning", "bytesin", "pagehits",
            "avgqueuetime", "errorcount", "instancename", "reqrunning", "wsreqtimedout",
            "cfcreqtimedout", "templatereqrunning", "avgdbtime", "cfcreqrunning",
            "templatereqqueued"
        };
        for (const char *k : keys) {
            cfvariant v(cfvariant::Long);
            v.m_long = 0;
            st.structSet(k, v);
        }
        st.structSet("instancename", cfvariant("cfserver"));
        return new cfvariant(st);
    }

    // Unknown modes return an empty struct (CF's perf metrics API behavior for
    // unrecognized modes is server-version specific).
    cfvariant st(cfvariant::Struct);
    return new cfvariant(st);
}

} // namespace cfml
