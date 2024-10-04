/**
 * @file tag_header.cpp
 * @brief <cfheader> runtime (response_add_header).
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>
#include <webstrada/config.h>
#include <webstrada/exceptions.h>
#include <webstrada/string.h>

#include <sys/stat.h>
#include <unistd.h>
#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace cfml {

void response_add_header(const cfvariant *name, const cfvariant *value,
                               const cfvariant *charset, const cfvariant *statusCode)
{
    auto &r = response();
    if (r.committed) {
        throw webstrada::exception("Failed to add HTML header. ColdFusion was unable to add the header you specified to the output stream. This is probably because you have already used a cfflush tag in your template or buffered output is turned off.");
    }
    if (statusCode) {
        r.statusCode = cfvariant_to_int(statusCode);
    }
    if (!name) return;
    string nameStr = variantToString(*name);
    if (!value) return;
    string valueStr = stripCRLF(variantToString(*value));

    string cs;
    if (charset) {
        cs = stripCRLF(variantToString(*charset));
    } else if (nameStr.compareCaseInsensitive("content-disposition") == 0) {
        cs = "UTF-8";
    }
    if (!cs.isEmpty()) {
        try {
            std::vector<std::byte> bytes;
            stringToBytes(valueStr, cs, bytes);
            valueStr.clear();
            if (!bytes.empty()) {
                valueStr.append(reinterpret_cast<const char*>(bytes.data()), bytes.size());
            }
        } catch (const webstrada::exception &) {
            throw webstrada::exception("Attribute validation error for tag cfheader. " + cs + " is not a supported character encoding.");
        }
    }

    if (nameStr.compareCaseInsensitive("content-type") == 0) {
        if (!r.committed) {
            string mime, cs2;
            parseContentType(valueStr, mime, cs2);
            if (!mime.isEmpty()) r.contentType = mime;
            if (!cs2.isEmpty()) {
                responseCharsetCanonical(cs2);
                r.charset = cs2;
            }
        }
    } else {
        r.headers.emplace_back(std::string(nameStr.constData(), nameStr.length()),
                               std::string(valueStr.constData(), valueStr.length()));
    }
}

} // namespace cfml
