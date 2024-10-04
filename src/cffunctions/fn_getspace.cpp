/**
 * @file fn_getspace.cpp
 * @brief CFML getfreespace() / gettotalspace() built-ins.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>
#include <webstrada/exceptions.h>
#include <webstrada/string.h>
#include <sys/statvfs.h>
#include <string>

namespace cfml {

static cfvariant *spaceValue(const cfvariant *path, bool wantFree, const char *funcName) {
    if (!path) throw webstrada::exception(webstrada::string(funcName) + " requires exactly 1 argument");
    string p = const_cast<cfvariant*>(path)->toString();

    // CF validates the path shape: it must be longer than one character and
    // start with '/', have a drive letter at position 1, or start with "\\"
    // (the decompiled CFPage.GetTotalSpace). A bare "/" fails this check on
    // CF 2025 (verified on the RDS host).
    bool validShape = p.length() > 1 &&
        (p.at(0) == '/' || (p.length() > 1 && p.at(1) == ':') ||
         p.startWith("\\"));
    if (!validShape) {
        webstrada::string msg("The value of the path parameter ");
        msg.append("\"");
        msg.append(p);
        msg.append("\" passed to ");
        msg.append(funcName);
        msg.append(" function is invalid.");
        throw webstrada::exception(msg);
    }

    struct statvfs sv;
    if (statvfs(p.constData(), &sv) != 0) {
        webstrada::string msg("The value of the path parameter ");
        msg.append("\"");
        msg.append(p);
        msg.append("\" passed to ");
        msg.append(funcName);
        msg.append(" function is invalid.");
        throw webstrada::exception(msg);
    }

    // Java File.getFreeSpace()/getTotalSpace() semantics: f_bavail * frsize
    // (free for unprivileged users) and f_blocks * frsize.
    unsigned long long bytes = wantFree
        ? (static_cast<unsigned long long>(sv.f_bavail) * sv.f_frsize)
        : (static_cast<unsigned long long>(sv.f_blocks) * sv.f_frsize);

    auto *ret = new cfvariant(cfvariant::Long);
    ret->m_long = static_cast<long long>(bytes);
    return ret;
}

cfvariant *cf_getfreespace(const cfvariant *path) {
    return spaceValue(path, true, "GETFREESPACE");
}

cfvariant *cf_gettotalspace(const cfvariant *path) {
    return spaceValue(path, false, "GETTOTALSPACE");
}

} // namespace cfml
