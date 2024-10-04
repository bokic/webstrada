/**
 * @file tag_content.cpp
 * @brief <cfcontent> runtime (response_apply_cfcontent).
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

void response_apply_cfcontent(string *out, const cfvariant *type, const cfvariant *reset,
                                    const cfvariant *file, const cfvariant *variable,
                                    const cfvariant *deletefile)
{
    auto &r = response();

    if (file || variable) {
        r.binary = true;
        if (out) out->clear();
        if (file) {
            string path = variantToString(*file);
            std::vector<std::byte> bytes;
            readFileBytes(path, bytes);
            if (out) out->append(reinterpret_cast<const char*>(bytes.data()), bytes.size());
            if (deletefile && cfmlBoolean(deletefile, false)) {
                unlink(path.constData());
            }
        } else {
            if (variable->m_type == cfvariant::Binary && variable->m_binary) {
                if (out) out->append(reinterpret_cast<const char*>(variable->m_binary->data()), variable->m_binary->size());
            } else {
                std::vector<char> bytes = encodeBuffer(variantToString(*variable), r);
                if (out) out->append(bytes.data(), bytes.size());
            }
        }
    } else {
        bool shouldReset = (reset == nullptr) || cfmlBoolean(reset, true);
        if (shouldReset) {
            // CF's <cfcontent> reset clears the WHOLE page output, not just the
            // buffer being written. Inside <cfsilent> the body writes to the
            // discard buffer, so the real (non-discard) page buffer must be
            // cleared too (verified on CF 2025: <cfoutput>A|</cfoutput>
            // <cfsilent><cfcontent type="text/plain"></cfsilent>
            // <cfoutput>|B</cfoutput> -> only "|B" with text/plain; was BUGS.md
            // "Output-affecting tags inside <cfsilent>").
            if (out) out->clear();
            if (string *realOut = silent_real_out()) {
                realOut->clear();
            }
        }
    }

    if (!r.committed && type) {
        string typeStr = variantToString(*type);
        string mime, charset;
        parseContentType(typeStr, mime, charset);
        if (!mime.isEmpty()) r.contentType = mime;
        if (!charset.isEmpty()) {
            responseCharsetCanonical(charset);
            r.charset = charset;
        }
    }
}

} // namespace cfml
