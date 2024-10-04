/**
 * @file fn_getsystemmemory.cpp
 * @brief CFML getsystemfreememory() / getsystemtotalmemory() built-ins.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>
#include <webstrada/exceptions.h>
#include <sys/sysinfo.h>
#include <cstdint>

namespace cfml {

cfvariant *cf_getsystemfreememory() {
    // The RDS host lacks the pmtagent package, so CF 2025 throws "The pmtagent
    // package is not installed." there (see BUGS_CF.md). Here we report the
    // OS free memory in bytes, per the documented semantics.
    struct sysinfo si;
    long long freeBytes = 0;
    if (sysinfo(&si) == 0) {
        unsigned long long unit = (si.mem_unit != 0) ? si.mem_unit : 1ULL;
        freeBytes = static_cast<long long>(si.freeram * unit);
    }
    auto *ret = new cfvariant(cfvariant::Long);
    ret->m_long = freeBytes;
    return ret;
}

cfvariant *cf_getsystemtotalmemory() {
    struct sysinfo si;
    long long totalBytes = 0;
    if (sysinfo(&si) == 0) {
        unsigned long long unit = (si.mem_unit != 0) ? si.mem_unit : 1ULL;
        totalBytes = static_cast<long long>(si.totalram * unit);
    }
    auto *ret = new cfvariant(cfvariant::Long);
    ret->m_long = totalBytes;
    return ret;
}

} // namespace cfml
