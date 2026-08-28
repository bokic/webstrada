/**
 * @file fn_requesttrace.cpp
 * @brief Compiler-extension __requestTrace(requestId) built-in.
 *
 * Queries WebStrada-profiler.sqlite and returns detailed line execution
 * steps for a specific request.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/profiler_store.h>

#include <cstdint>
#include <vector>

namespace cfml {

cfvariant *cf___requesttrace(const cfvariant **args, int argc)
{
    int64_t reqId = 0;
    if (argc >= 1 && args && args[0]) {
        reqId = toInt(args[0]);
    }

    cfvariant root(cfvariant::Struct);
    cfvariant idVar(cfvariant::Long);
    idVar.m_long = reqId;
    root.structSet("requestId", idVar);

    cfvariant stepsArr(cfvariant::Array);

    webstrada::open_profiler_store();
    std::vector<webstrada::TraceStep> steps;
    if (webstrada::profiler_store().isOpen() && reqId > 0) {
        webstrada::profiler_store().getRequestSteps(reqId, steps);
    }

    for (const auto &step : steps) {
        cfvariant row(cfvariant::Struct);
        cfvariant seqVar(cfvariant::Number);
        seqVar.m_int = step.seq;
        row.structSet("seq", seqVar);
        row.structSet("type", cfvariant(step.type.c_str()));
        row.structSet("path", cfvariant(step.path.c_str()));
        row.structSet("function", cfvariant(step.function.c_str()));
        cfvariant line(cfvariant::Number);
        line.m_int = step.line;
        row.structSet("line", line);
        row.structSet("stackTrace", cfvariant(step.stackTrace.c_str()));
        cfvariant dVar(cfvariant::Float);
        dVar.m_double = step.durationMs;
        row.structSet("durationMs", dVar);
        row.structSet("deltaMs", dVar);
        cfvariant tsVar(cfvariant::Float);
        tsVar.m_double = step.timestampMs;
        row.structSet("timestampMs", tsVar);
        row.structSet("elapsedMs", tsVar);

        stepsArr.m_array->push_back(row);
    }
    root.structSet("steps", stepsArr);

    return new cfvariant(root);
}

} // namespace cfml
