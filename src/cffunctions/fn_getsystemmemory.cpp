/**
 * @file fn_getsystemmemory.cpp
 * @brief CFML getsystemfreememory() / getsystemtotalmemory() built-ins.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>
#include <webstrada/exceptions.h>
#include <cstdint>

#ifdef __APPLE__
#include <sys/types.h>
#include <sys/sysctl.h>
#include <mach/mach.h>
#else
#include <sys/sysinfo.h>
#endif

namespace cfml {

cfvariant *cf_getsystemfreememory() {
    // The RDS host lacks the pmtagent package, so CF 2025 throws "The pmtagent
    // package is not installed." there (see BUGS_CF.md). Here we report the
    // OS free memory in bytes, per the documented semantics.
    long long freeBytes = 0;
#ifdef __APPLE__
    mach_port_t host = mach_host_self();
    vm_size_t pageSize = 0;
    host_page_size(host, &pageSize);
    vm_statistics64_data_t stats;
    mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;
    if (host_statistics64(host, HOST_VM_INFO64, reinterpret_cast<host_info64_t>(&stats), &count) == KERN_SUCCESS) {
        freeBytes = static_cast<long long>((stats.free_count + stats.active_count) * pageSize);
    }
#else
    struct sysinfo si;
    if (sysinfo(&si) == 0) {
        unsigned long long unit = (si.mem_unit != 0) ? si.mem_unit : 1ULL;
        freeBytes = static_cast<long long>(si.freeram * unit);
    }
#endif
    auto *ret = new cfvariant(cfvariant::Long);
    ret->m_long = freeBytes;
    return ret;
}

cfvariant *cf_getsystemtotalmemory() {
    long long totalBytes = 0;
#ifdef __APPLE__
    int mib[2] = { CTL_HW, HW_MEMSIZE };
    unsigned long long memsize = 0;
    size_t len = sizeof(memsize);
    if (sysctl(mib, 2, &memsize, &len, nullptr, 0) == 0) {
        totalBytes = static_cast<long long>(memsize);
    }
#else
    struct sysinfo si;
    if (sysinfo(&si) == 0) {
        unsigned long long unit = (si.mem_unit != 0) ? si.mem_unit : 1ULL;
        totalBytes = static_cast<long long>(si.totalram * unit);
    }
#endif
    auto *ret = new cfvariant(cfvariant::Long);
    ret->m_long = totalBytes;
    return ret;
}

} // namespace cfml
