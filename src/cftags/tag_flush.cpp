/**
 * @file tag_flush.cpp
 * @brief <cfflush> runtime (response_flush).
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

void response_flush(string *out)
{
    // CF commits the response on the FIRST <cfflush> even when the buffer is
    // empty (verified on CF 2025: an empty flush before a later <cfcontent>
    // keeps the earlier charset — the empty flush locked the headers). So an
    // empty flush still sends the headers / marks the response committed.
    if (silent_buf_contains(out)) return;
    auto &r = response();
    if (!r.stream) {
        if (out && !out->isEmpty()) g_cli_flushed.append(*out);
        r.committed = true;
        if (out) out->clear();
        return;
    }
    if (!r.committed) {
        sendHeader(r);
        r.committed = true;
    }
    if (out && !out->isEmpty()) {
        sendEncoded(*out, r);
        out->clear();
    }
}

} // namespace cfml
