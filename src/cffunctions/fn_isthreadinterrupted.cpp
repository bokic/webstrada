/**
 * @file fn_isthreadinterrupted.cpp
 * @brief CFML isthreadinterrupted() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>
#include <webstrada/exceptions.h>
#include <webstrada/string.h>

namespace cfml {

cfvariant *cf_isthreadinterrupted(const cfvariant *threadName) {
    if (!threadName) throw webstrada::exception("isThreadInterrupted requires exactly 1 argument");
    // CF 2025 takes the thread name (verified on the RDS host: 0 args reports
    // "Parameter validation error ... The function takes 1 parameter."). This
    // engine has no thread subsystem (cfthread is unimplemented), so no thread
    // can exist; CF's "thread was not created" error is reproduced.
    webstrada::string tname = const_cast<cfvariant*>(threadName)->toString();
    webstrada::string msg("Thread ");
    msg.append(tname);
    msg.append(" cannot be used in action isInterrupted because the thread ");
    msg.append(tname);
    msg.append(" was not created.");
    throw webstrada::exception(msg);
}

} // namespace cfml
