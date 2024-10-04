/**
 * @file fn_callstackdump.cpp
 * @brief CFML CallStackDump() built-in.
 *
 * Outputs a text representation of the live CFML call stack (cfml::g_callStack):
 * one line per frame, innermost first. A function/method frame renders as
 * `<template>:<FUNCTION>:<line>`, a plain page frame as `<template>:<line>`,
 * each terminated by a newline (verified against CF 2025 on the RDS host).
 * With no argument (or "browser") the dump is appended to the page output;
 * a path argument writes the dump to that file instead.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>

#include <filesystem>
#include <fstream>

namespace cfml {

cfvariant *cf_callstackdump(string *out, const cfvariant *output) {
    string dump;
    for (auto it = g_callStack.rbegin(); it != g_callStack.rend(); ++it) {
        dump += webstrada::string(it->path.c_str());
        if (!it->function.empty()) {
            dump += ":";
            dump += webstrada::string(it->function.c_str());
        }
        dump += ":";
        dump += webstrada::string::number(it->line);
        dump += "\n";
    }

    std::string target;
    if (output) {
        webstrada::string s = const_cast<cfvariant*>(output)->toString();
        const char *d = s.constData();
        target.assign(d ? d : "", d ? s.length() : 0);
    }

    if (target.empty() || target == "browser") {
        if (out) out->append(dump);
        return nullptr;
    }

    // A path argument appends the dump to that file (CF 2025 appends rather
    // than truncates; a bare "file" is treated like a relative path "file").
    std::filesystem::path p(target);
    if (p.has_parent_path()) {
        std::error_code ec;
        std::filesystem::create_directories(p.parent_path(), ec);
    }
    std::ofstream f(target, std::ios::binary | std::ios::app);
    if (!f) {
        throw webstrada::exception(string(("CallStackDump: could not write to " + target).c_str()));
    }
    f.write(dump.constData(), dump.length());
    return nullptr;
}

} // namespace cfml
