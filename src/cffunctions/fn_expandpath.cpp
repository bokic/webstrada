/**
 * @file fn_expandpath.cpp
 * @brief CFML expandpath() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>
#include <webstrada/exceptions.h>
#include <webstrada/string.h>
#include <webstrada/upload.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cctype>
#include <cstring>
#include <ctime>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

using webstrada::UploadedFile;

namespace cfml {

cfvariant *cf_expandpath(const cfvariant *path) {
    if (!path) throw webstrada::exception("ExpandPath: Missing argument");
    string pStr = const_cast<cfvariant*>(path)->toString();

    if (pStr.isEmpty()) {
        auto *ret = new cfvariant(string(""));
        return ret;
    }

    // CF's ExpandPath resolves relative to the current template path directory (or current working directory).
    // Leading "/" is treated as relative to the web root.
    IncludeRuntime *rt = include_context();
    std::filesystem::path base = std::filesystem::current_path();
    const char *inputCStr = pStr.constData();
    bool isRootRelative = (!pStr.isEmpty() && (inputCStr[0] == '/' || inputCStr[0] == '\\'));
    bool hasTrailingSlash = (inputCStr[pStr.length() - 1] == '/' || inputCStr[pStr.length() - 1] == '\\');

    if (isRootRelative) {
        if (rt && !rt->webRoot.empty()) {
            base = std::filesystem::path(rt->webRoot);
        }
    } else {
        if (rt && !rt->currentPath.empty()) {
            std::filesystem::path cur(rt->currentPath);
            if (cur.has_parent_path()) {
                base = cur.parent_path();
            }
        }
    }

    // Strip leading root separators so the path resolves relative to base
    std::filesystem::path input(inputCStr);
    std::filesystem::path relInput = input.relative_path();

    std::filesystem::path full = base / relInput;
    std::filesystem::path normalized = full.lexically_normal();

    std::string result = normalized.string();

    // Remove trailing slash added by normalization of "." / ".." unless input had one
    if (!hasTrailingSlash && result.size() > 1 && result.back() == '/') {
        result.pop_back();
    }

    // Preserve trailing slash exactly like CF does
    if (hasTrailingSlash && !result.empty() && result.back() != '/') {
        result += '/';
    }

    auto *ret = new cfvariant(string(result.c_str()));
    return ret;
}

} // namespace cfml
