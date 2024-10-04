/**
 * @file fn_getcpuusage.cpp
 * @brief CFML getcpuusage() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>
#include <webstrada/exceptions.h>
#include <webstrada/string.h>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <thread>

namespace cfml {

namespace {

// Reads the aggregate CPU jiffies from /proc/stat (all cores) plus the idle
// jiffies. Returns false when /proc/stat is unavailable.
bool readCpuTicks(unsigned long long &total, unsigned long long &idle) {
    std::ifstream in("/proc/stat");
    std::string line;
    if (!std::getline(in, line)) return false;
    if (line.rfind("cpu ", 0) != 0) return false;
    // cpu user nice system idle iowait irq softirq steal guest guest_nice
    unsigned long long vals[10] = {0};
    int n = std::sscanf(line.c_str(), "cpu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu",
                        &vals[0], &vals[1], &vals[2], &vals[3], &vals[4],
                        &vals[5], &vals[6], &vals[7], &vals[8], &vals[9]);
    if (n < 4) return false;
    total = 0;
    for (int i = 0; i < 10; i++) total += vals[i];
    idle = vals[3] + (n > 4 ? vals[4] : 0); // idle + iowait
    return true;
}

} // namespace

cfvariant *cf_getcpuusage(const cfvariant *interval) {
    long long ms = 1000;
    if (interval) {
        double d = getDoubleValue(*const_cast<cfvariant*>(interval));
        ms = static_cast<long long>(d);
        if (ms < 0) ms = 0;
    }

    unsigned long long t1 = 0, i1 = 0;
    if (!readCpuTicks(t1, i1)) {
        // No /proc/stat (unexpected on Linux). CF's host throws the pmtagent
        // "not installed" error; report 0 so callers keep working.
        auto *ret = new cfvariant(cfvariant::Float);
        ret->m_double = 0.0;
        return ret;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(ms));

    unsigned long long t2 = 0, i2 = 0;
    if (!readCpuTicks(t2, i2)) {
        auto *ret = new cfvariant(cfvariant::Float);
        ret->m_double = 0.0;
        return ret;
    }

    unsigned long long dTotal = t2 > t1 ? t2 - t1 : 0;
    unsigned long long dIdle = i2 > i1 ? i2 - i1 : 0;
    double usage = (dTotal == 0) ? 0.0 : (double)(dTotal - dIdle) / (double)dTotal;

    auto *ret = new cfvariant(cfvariant::Float);
    ret->m_double = usage * 100.0;
    return ret;
}

} // namespace cfml
