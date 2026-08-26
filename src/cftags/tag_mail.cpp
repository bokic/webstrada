/**
 * @file tag_mail.cpp
 * @brief Non-delivering <cfmail> runtime.
 */

#include "common.h"

#include <webstrada/cfvariant.h>

#include <cstdio>
#include <string>

namespace cfml {

namespace {
std::string renderAttrs(const cfvariant *attrs)
{
    if (!attrs || attrs->m_type != cfvariant::Struct || !attrs->m_struct) return "";
    std::string out;
    for (const auto &kv : *attrs->m_struct) {
        if (!out.empty()) out += ", ";
        out += kv.first.constData();
        out += "=";
        out += safe_to_std_string(kv.second);
    }
    return out;
}
}

void cf_mail_tag(const cfvariant *attrs)
{
    std::string detail = renderAttrs(attrs);
    if (detail.empty()) {
        fprintf(stderr, "[WebStrada] <cfmail> is not implemented; call logged (no attributes).\n");
    } else {
        fprintf(stderr, "[WebStrada] <cfmail> is not implemented; call logged (%s).\n", detail.c_str());
    }
}

static void logMailChild(const char *tag, const cfvariant *attrs)
{
    std::string detail = renderAttrs(attrs);
    if (detail.empty()) {
        fprintf(stderr, "[WebStrada] <%s> is not implemented; call logged (no attributes).\n", tag);
    } else {
        fprintf(stderr, "[WebStrada] <%s> is not implemented; call logged (%s).\n", tag, detail.c_str());
    }
}

void cf_mailpart_tag(const cfvariant *attrs)
{
    logMailChild("cfmailpart", attrs);
}

void cf_mailparam_tag(const cfvariant *attrs)
{
    logMailChild("cfmailparam", attrs);
}

}
