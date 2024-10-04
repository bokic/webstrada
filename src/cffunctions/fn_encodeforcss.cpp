/**
 * @file fn_encodeforcss.cpp
 * @brief CFML encodeforcss() built-in.
 */

#include "common.h"

#include "fn_esapi.h"

#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>
#include <webstrada/exceptions.h>
#include <webstrada/string.h>
#include <regex>
#include <string>

namespace cfml {

// CSSCodec.encode preserves rgb(...) triplets verbatim before encoding the rest
// (ESAPI's EncodingPatternPreservation). The pattern is
//   ([rR][gG][bB])\s*\(\s*\d{1,3}\s*(\%)?\s*,\s*\d{1,3}\s*(\%)?\s*,\s*\d{1,3}\s*(\%)?\s*\)
// Each match is replaced by a marker, the remainder is encoded, then the
// originals are restored (replacement markers are restored one at a time, so a
// marker string is used that cannot appear in normal CSS-encoded output).
static const std::string &cssRgbPattern() {
    static const std::string kPat =
        "([rR][gG][bB])\\s*\\(\\s*\\d{1,3}\\s*(%)?\\s*,\\s*\\d{1,3}\\s*(%)?\\s*,\\s*\\d{1,3}\\s*(%)?\\s*\\)";
    return kPat;
}

cfvariant *cf_encodeforcss(const cfvariant *str, const cfvariant *canonicalize) {
    if (!str) throw webstrada::exception("EncodeForCSS requires 1 argument");
    string input = const_cast<cfvariant*>(str)->toString();
    std::string in = input.constData() ? input.constData() : "";
    if (canonicalize && cf_is_truthy_value(canonicalize)) {
        in = esapiCanonicalizeCatch(in, false, false, false);
    }
    // IMMUNE_CSS = {'#'} when esapi.cssencoder.encodehash is not set (the CF
    // default; verified against CF 2025 on the RDS host).
    static const char kImmune[] = {'#'};

    // Preserve rgb(...) triplets.
    const std::string &pat = cssRgbPattern();
    std::vector<std::string> preserved;
    std::string working;
    std::regex re(pat);
    std::sregex_iterator it(in.begin(), in.end(), re), endIt;
    size_t last = 0;
    const std::string marker = "ESAPI_ENCODING_PRESERVATION_MARKER";
    for (; it != endIt; ++it) {
        working += in.substr(last, it->position() - last);
        preserved.push_back(it->str());
        working += marker;
        last = it->position() + it->length();
    }
    working += in.substr(last);

    std::string out = esapiEncodeString(working, esapiEncodeCssCodePoint, kImmune, sizeof(kImmune));

    // Restore originals in order.
    for (const auto &orig : preserved) {
        size_t pos = out.find(marker);
        if (pos != std::string::npos) {
            out.replace(pos, marker.length(), orig);
        }
    }
    return new cfvariant(out.c_str());
}

} // namespace cfml
