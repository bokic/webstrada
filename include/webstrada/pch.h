// Precompiled header for the WebStrada codebase.
//
// Contains only *stable* headers: the C++ standard library, the C system
// libraries used by the runtime, and the project's core headers. Frequently
// edited headers (cffunctions/common.h, cftags/common.h, core_internal.h)
// are intentionally NOT included here: putting them here would invalidate the
// PCH on every edit and serialize a full target rebuild through a single PCH
// compilation step.
#pragma once

// ---- C++ standard library ----
#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <climits>
#include <cmath>
#include <compare>
#include <csetjmp>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <filesystem>
#include <format>
#include <fstream>
#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <print>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

// ---- C system libraries used by the runtime ----
// pcre2.h is intentionally NOT precompiled: its API is selected by the
// PCRE2_CODE_UNIT_WIDTH macro at include time, so it must stay per-TU.
#include <json-c/json.h>
#include <sqlite3.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/hmac.h>
#include <openssl/provider.h>
#include <cairo.h>
#include <jpeglib.h>
#include <zlib.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <libxslt/xslt.h>
#include <libxslt/xsltInternals.h>
#include <libxslt/transform.h>
#include <libxslt/xsltutils.h>
#include <libxml/xmlschemas.h>

// ---- project core headers (stable) ----
#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>
#include <webstrada/string.h>
#include <webstrada/exceptions.h>
#include <webstrada/config.h>
#include <webstrada/cfimage.h>
#include <webstrada/locale.h>
#include <webstrada/upload.h>
