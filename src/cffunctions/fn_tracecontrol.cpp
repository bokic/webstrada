/**
 * @file fn_tracecontrol.cpp
 * @brief Compiler-extension __traceControl(action) built-in.
 *
 * Controls execution tracing session state:
 *   - "start": clears tracing database and memory buffer, resets session counter, enables lineExecutionTrace
 *   - "stop": disables lineExecutionTrace
 *   - "clear": clears tracing database and memory buffer, resets session counter
 *   - "status" (or default): returns current tracing status and session count
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/profiler_store.h>
#include <webstrada/server_stats.h>
#include <webstrada/config.h>

#include <string>
#include <cctype>

namespace cfml {

cfvariant *cf___tracecontrol(const cfvariant **args, int argc)
{
    std::string action = "status";
    if (argc >= 1 && args && args[0]) {
        action = const_cast<cfvariant*>(args[0])->toString().constData() ? const_cast<cfvariant*>(args[0])->toString().constData() : "";
        for (auto &c : action) {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
    }

    cfvariant root(cfvariant::Struct);

    auto setBool = [&](const char *key, bool val) {
        cfvariant v(cfvariant::Boolean);
        v.m_bool = val;
        root.structSet(key, v);
    };

    if (action == "start") {
        webstrada::open_profiler_store();
        if (webstrada::profiler_store().isOpen()) {
            webstrada::profiler_store().clear();
        }
        webstrada::stats::clear_recent_requests();
        webstrada::stats::reset_trace_session_count();
        webstrada::config::lineExecutionTrace = true;
        setBool("ok", true);
        setBool("lineExecutionTrace", true);
        cfvariant tsc(cfvariant::Number);
        tsc.m_int = 0;
        root.structSet("traceSessionCount", tsc);
    } else if (action == "stop") {
        webstrada::config::lineExecutionTrace = false;
        setBool("ok", true);
        setBool("lineExecutionTrace", false);
        cfvariant tsc(cfvariant::Number);
        tsc.m_int = webstrada::stats::trace_session_count();
        root.structSet("traceSessionCount", tsc);
    } else if (action == "clear") {
        webstrada::open_profiler_store();
        if (webstrada::profiler_store().isOpen()) {
            webstrada::profiler_store().clear();
        }
        webstrada::stats::clear_recent_requests();
        webstrada::stats::reset_trace_session_count();
        setBool("ok", true);
        setBool("lineExecutionTrace", webstrada::config::lineExecutionTrace);
        cfvariant tsc(cfvariant::Number);
        tsc.m_int = 0;
        root.structSet("traceSessionCount", tsc);
    } else if (action == "set_hide_admin" || action == "sethideadmin") {
        bool hide = true;
        if (argc >= 2 && args && args[1]) {
            hide = cfvariant_is_truthy(args[1]);
        }
        webstrada::stats::set_hide_admin_requests(hide);
        setBool("ok", true);
        setBool("lineExecutionTrace", webstrada::config::lineExecutionTrace);
        cfvariant tsc(cfvariant::Number);
        tsc.m_int = webstrada::stats::trace_session_count();
        root.structSet("traceSessionCount", tsc);
    } else {
        setBool("ok", true);
        setBool("lineExecutionTrace", webstrada::config::lineExecutionTrace);
        cfvariant tsc(cfvariant::Number);
        tsc.m_int = webstrada::stats::trace_session_count();
        root.structSet("traceSessionCount", tsc);
    }

    setBool("hideAdminRequests", webstrada::stats::hide_admin_requests());

    return new cfvariant(root);
}

} // namespace cfml
