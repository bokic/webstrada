/**
 * @file tag_ftp.cpp
 * @brief <cfftp> runtime (cf_ftp_tag).
 *
 * cfftp is deliberately NOT implemented (no FTP client is wired in). Per
 * project convention the compiler still validates the attribute set like CF
 * and emits a call to this runtime, which only logs the call (tag name + all
 * evaluated attributes) to the engine log on stderr and performs no FTP
 * operation. This lets templates that use <cfftp> compile and run instead of
 * failing with "Tag cfftp is not implemented".
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

void cf_ftp_tag(const cfvariant *attrs)
{
    std::string detail = renderAttrs(attrs);
    if (detail.empty()) {
        fprintf(stderr, "[WebStrada] <cfftp> is not implemented; call logged (no attributes).\n");
    } else {
        fprintf(stderr, "[WebStrada] <cfftp> is not implemented; call logged (%s).\n", detail.c_str());
    }
}

} // namespace cfml
