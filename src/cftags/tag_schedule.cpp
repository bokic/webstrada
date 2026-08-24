/**
 * @file tag_schedule.cpp
 * @brief <cfschedule> runtime (cf_schedule_tag).
 *
 * cfschedule is deliberately NOT implemented (no scheduler engine is wired
 * in). Per project convention the compiler still validates the attribute set
 * like CF and emits a call to this runtime, which only logs the call (tag
 * name + all evaluated attributes) to the engine log on stderr and performs
 * no scheduling. This lets templates that use <cfschedule> compile and run
 * instead of failing with "Tag cfschedule is not implemented".
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>
#include <webstrada/exceptions.h>
#include <webstrada/string.h>

#include <cstdio>
#include <string>

namespace cfml {

namespace {

// Renders "key=value, ..." for every present attribute (lowercased keys, like
// the compiled struct), for the log line.
std::string renderAttrs(const cfvariant *attrs)
{
    if (!attrs || attrs->m_type != cfvariant::Struct || !attrs->m_struct) return "";
    std::string out;
    for (const auto &kv : *attrs->m_struct) {
        std::string value = safe_to_std_string(kv.second);
        if (!out.empty()) out += ", ";
        out += kv.first.constData();
        out += "=";
        out += value;
    }
    return out;
}

} // namespace

void cf_schedule_tag(const cfvariant *attrs)
{
    std::string detail = renderAttrs(attrs);
    if (detail.empty()) {
        fprintf(stderr, "[WebStrada] <cfschedule> is not implemented; call logged (no attributes).\n");
    } else {
        fprintf(stderr, "[WebStrada] <cfschedule> is not implemented; call logged (%s).\n", detail.c_str());
    }
}

} // namespace cfml
