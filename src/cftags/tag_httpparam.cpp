/**
 * @file tag_httpparam.cpp
 * @brief <cfhttpparam> runtime (cf_http_param).
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>
#include <webstrada/exceptions.h>
#include <webstrada/string.h>

#include <string>
#include <vector>

namespace cfml {

void cf_http_param(const cfvariant *type, const cfvariant *name, const cfvariant *value,
                         const cfvariant *file, const cfvariant *encoded, const cfvariant *mimetype)
{
    if (g_httpCtxs.empty()) {
        throw webstrada::exception("cfhttpparam is only valid inside a cfhttp tag.");
    }
    HttpParam p;
    if (type) p.type = safe_to_std_string(*type);
    if (name) p.name = safe_to_std_string(*name);
    if (value) p.value = safe_to_std_string(*value);
    if (file) p.file = safe_to_std_string(*file);
    if (mimetype) p.mimetype = safe_to_std_string(*mimetype);
    if (encoded) p.encoded = isTruthy(*encoded);
    g_httpCtxs.back()->params.push_back(std::move(p));
}

} // namespace cfml
