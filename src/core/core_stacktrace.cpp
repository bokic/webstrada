#include "core_internal.h"
#include "../cftags/common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <webstrada/parser.h>
#include <webstrada/worker.h>
#include <webstrada/cfimage.h>
#include <webstrada/cfvariant.h>
#include <webstrada/string.h>
#include <webstrada/scope_store.h>
#include <webstrada/config.h>
#include <webstrada/locale.h>
#include <webstrada/cfimage.h>

#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>
#include <sqlite3.h>
#include <curl/curl.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/hmac.h>
#include <openssl/provider.h>

#include <thread>
#include <chrono>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <ctime>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <fstream>
#include <filesystem>
#include <unistd.h>
#include <fcntl.h>

using namespace webstrada;
using namespace cfml;
// ---- CFML call stack (stacktrace) ----
//
// The live call stack tracks every executing template / UDF / component-method
// / component-construction frame as {full pathname, current line}. The JIT
// emits cf_stack_push at each frame entry, cf_stack_set_line before every
// statement and cf_stack_pop on every exit path. Because the compiled functions
// are plain C++-ABI entry points, the stack lives in a thread-local (like
// g_udfCtx) so a whole request is one thread and helper calls cannot pass it
// explicitly.
//
// When an exception unwinds, the FIRST landing pad the exception reaches (the
// innermost function boundary or try/catch that is still on the way out)
// snapshots the live stack into the in-flight exception's m_stackTrace. Later
// landing pads skip it (hasStackTrace), so the snapshot always carries the exact
// frame chain + line where the error occurred, exactly like a Java
// Throwable.getStackTrace(). The landing pads then pop the frames, so after a
// catch the live stack is balanced again and later errors report cleanly.

namespace cfml {

thread_local std::vector<webstrada::StackLevel> g_callStack;
static thread_local std::chrono::steady_clock::time_point s_lastTraceTime;
static thread_local bool s_traceTimerStarted = false;

static thread_local std::vector<webstrada::TraceStep> g_requestTraceSteps;
static thread_local std::chrono::steady_clock::time_point g_requestTraceStartTime;
static thread_local bool g_requestTraceActive = false;

void trace_begin_request()
{
    g_requestTraceSteps.clear();
    g_requestTraceSteps.reserve(2048);
    g_requestTraceStartTime = std::chrono::steady_clock::now();
    s_lastTraceTime = g_requestTraceStartTime;
    s_traceTimerStarted = true;
    g_requestTraceActive = true;
}

std::vector<webstrada::TraceStep> trace_take_steps()
{
    std::vector<webstrada::TraceStep> res;
    res.swap(g_requestTraceSteps);
    g_requestTraceActive = false;
    return res;
}

static std::string currentStackTraceString()
{
    if (g_callStack.empty()) return "";
    std::string s;
    for (size_t i = 0; i < g_callStack.size(); ++i) {
        if (i > 0) s += "|";
        const auto &lvl = g_callStack[i];
        s += lvl.path;
        s += ":";
        s += std::to_string(lvl.line);
        if (!lvl.function.empty()) {
            s += ":";
            s += lvl.function;
        }
    }
    return s;
}

void trace_record_event(const char *type, const char *path, const char *function, int line)
{
    if (__builtin_expect(webstrada::config::lineExecutionTrace && g_requestTraceActive, 0)) {
        auto now = std::chrono::steady_clock::now();
        double deltaMs = s_traceTimerStarted ? std::chrono::duration<double, std::milli>(now - s_lastTraceTime).count() : 0.0;
        double elapsedMs = std::chrono::duration<double, std::milli>(now - g_requestTraceStartTime).count();
        s_lastTraceTime = now;
        s_traceTimerStarted = true;
        webstrada::TraceStep step;
        step.type = type ? type : "EVENT";
        step.path = path ? path : "[ENGINE]";
        step.function = function ? function : "";
        step.line = line;
        step.stackTrace = currentStackTraceString();
        step.deltaMs = deltaMs;
        step.elapsedMs = elapsedMs;
        g_requestTraceSteps.push_back(std::move(step));
    }
}

void cf_stack_push(const char *path, const char *function)
{
    webstrada::StackLevel lvl;
    lvl.path = path ? path : "";
    lvl.line = 0;
    if (function && *function) {
        lvl.function.assign(function);
        for (auto &c : lvl.function) c = static_cast<char>(toupper(static_cast<unsigned char>(c)));
    }
    g_callStack.push_back(std::move(lvl));

    if (__builtin_expect(webstrada::config::lineExecutionTrace && g_requestTraceActive, 0)) {
        auto now = std::chrono::steady_clock::now();
        double deltaMs = s_traceTimerStarted ? std::chrono::duration<double, std::milli>(now - s_lastTraceTime).count() : 0.0;
        double elapsedMs = std::chrono::duration<double, std::milli>(now - g_requestTraceStartTime).count();
        s_lastTraceTime = now;
        s_traceTimerStarted = true;
        const auto &top = g_callStack.back();
        webstrada::TraceStep step;
        step.type = "ENTRY";
        step.path = top.path;
        step.function = top.function;
        step.line = 0;
        step.stackTrace = currentStackTraceString();
        step.deltaMs = deltaMs;
        step.elapsedMs = elapsedMs;
        g_requestTraceSteps.push_back(std::move(step));
    }
}

void cf_stack_set_line(int line)
{
    if (g_callStack.empty()) return;
    auto &top = g_callStack.back();
    if (top.line == line) return;
    top.line = line;

    if (__builtin_expect(webstrada::config::lineExecutionTrace && g_requestTraceActive, 0)) {
        auto now = std::chrono::steady_clock::now();
        double deltaMs = s_traceTimerStarted ? std::chrono::duration<double, std::milli>(now - s_lastTraceTime).count() : 0.0;
        double elapsedMs = std::chrono::duration<double, std::milli>(now - g_requestTraceStartTime).count();
        s_lastTraceTime = now;
        s_traceTimerStarted = true;
        webstrada::TraceStep step;
        step.type = "LINE";
        step.path = top.path;
        step.function = top.function;
        step.line = line;
        step.stackTrace = currentStackTraceString();
        step.deltaMs = deltaMs;
        step.elapsedMs = elapsedMs;
        g_requestTraceSteps.push_back(std::move(step));
    }
}

void cf_stack_pop()
{
    if (__builtin_expect(webstrada::config::lineExecutionTrace && g_requestTraceActive, 0) && !g_callStack.empty()) {
        auto now = std::chrono::steady_clock::now();
        double deltaMs = s_traceTimerStarted ? std::chrono::duration<double, std::milli>(now - s_lastTraceTime).count() : 0.0;
        double elapsedMs = std::chrono::duration<double, std::milli>(now - g_requestTraceStartTime).count();
        s_lastTraceTime = now;
        s_traceTimerStarted = true;
        const auto &top = g_callStack.back();
        webstrada::TraceStep step;
        step.type = "EXIT";
        step.path = top.path;
        step.function = top.function;
        step.line = top.line;
        step.stackTrace = currentStackTraceString();
        step.deltaMs = deltaMs;
        step.elapsedMs = elapsedMs;
        g_requestTraceSteps.push_back(std::move(step));
    }
    if (!g_callStack.empty()) g_callStack.pop_back();
}

void cf_stack_capture_on_exception(void *exn)
{
    if (!exn) return;
    void *obj = abi::__cxa_get_exception_ptr(exn);
    if (!obj) return;
    const webstrada::exception *e = static_cast<const webstrada::exception*>(obj);
    if (!e || !e->m_stackTrace.empty()) return;
    const_cast<webstrada::exception*>(e)->m_stackTrace = g_callStack;

    // Control-flow signals like <cfexit> and <cfabort> are uncatchable
    // exceptions (catchable() == false) used for normal flow control, not
    // errors. Do not log them to stderr.
    if (!e->catchable()) return;

    // Log the exception to stderr / dev server log. Never stdout: stdout
    // carries the template's response payload, and verify_with_coldfusion.py
    // compares it byte-for-byte against Adobe CF.
    std::string loc = "";
    if (!g_callStack.empty()) {
        const auto &top = g_callStack.back();
        loc = " at " + top.path + ":" + std::to_string(top.line);
        if (!top.function.empty()) loc += " in " + top.function + "()";
    }
    fprintf(stderr, "[WebStrada][Exception] [%s] %s%s%s%s\n",
            e->m_type.constData() ? e->m_type.constData() : "Expression",
            e->m_message.constData() ? e->m_message.constData() : "",
            e->m_detail.isEmpty() ? "" : " | Detail: ",
            e->m_detail.constData() ? e->m_detail.constData() : "",
            loc.c_str());
    fflush(stderr);
}

// Builds a TAGCONTEXT array (element [1] = innermost frame) from a captured
// stack trace. The caller owns the returned variant (structSet copies it).
webstrada::cfvariant *cf_stack_tagcontext(const std::vector<webstrada::StackLevel> &st)
{
    auto *tags = new cfvariant(cfvariant::Array);
    // CF's tagContext array lists the innermost frame (where the error
    // occurred) first; the captured stack is stored outermost-first.
    for (auto it = st.rbegin(); it != st.rend(); ++it) {
        auto *frame = new cfvariant(cfvariant::Struct);
        frame->structSet("TEMPLATE", cfvariant(webstrada::string(it->path.c_str())));
        frame->structSet("LINE", cfvariant(it->line));
        tags->insert(*frame);
        delete frame;
    }
    return tags;
}

// Reverse of cf_stack_tagcontext: reads a TAGCONTEXT array (innermost-first)
// back into an outermost-first StackLevel vector, used by cf_eh_throw so a
// rethrow keeps the original stack trace (CF rethrows the same Java exception).
std::vector<webstrada::StackLevel> cf_stack_tagcontext_to_levels(const webstrada::cfvariant *tags)
{
    std::vector<webstrada::StackLevel> out;
    if (!tags || tags->m_type != cfvariant::Array || !tags->m_array) return out;
    std::vector<webstrada::StackLevel> innermostFirst;
    innermostFirst.reserve(tags->m_array->size());
    for (auto &elem : *tags->m_array) {
        webstrada::StackLevel lvl;
        if (elem.m_type == cfvariant::Struct && elem.m_struct) {
            auto tmplIt = elem.m_struct->find("TEMPLATE");
            if (tmplIt != elem.m_struct->end()) {
                webstrada::string s = tmplIt->second.toString();
                const char *d = s.constData();
                lvl.path.assign(d ? d : "", d ? s.length() : 0);
            }
            auto lineIt = elem.m_struct->find("LINE");
            if (lineIt != elem.m_struct->end()) {
                lvl.line = (lineIt->second.m_type == cfvariant::Number) ? lineIt->second.m_int : 0;
            }
        }
        innermostFirst.push_back(std::move(lvl));
    }
    for (auto it = innermostFirst.rbegin(); it != innermostFirst.rend(); ++it) {
        out.push_back(*it);
    }
    return out;
}

} // namespace cfml
