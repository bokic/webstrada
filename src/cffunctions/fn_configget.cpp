/**
 * @file fn_configget.cpp
 * @brief Compiler-extension __configGet() built-in.
 *
 * Returns the effective server configuration as a struct
 * { settings: {...}, datasources: {...} }, read straight from the
 * webstrada::config::* globals. Datasource passwords are masked with the
 * "****" sentinel so they never leave the server (__configSet() treats that
 * sentinel as "keep the existing password").
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/config.h>

#include <map>
#include <string>

namespace cfml {

static void putBool(cfvariant &st, const char *key, bool value)
{
    cfvariant v(cfvariant::Boolean);
    v.m_bool = value;
    st.structSet(key, v);
}

static void putInt(cfvariant &st, const char *key, int value)
{
    st.structSet(key, cfvariant(value));
}

static void putDouble(cfvariant &st, const char *key, double value)
{
    cfvariant v(cfvariant::Float);
    v.m_double = value;
    st.structSet(key, v);
}

static void putString(cfvariant &st, const char *key, const std::string &value)
{
    st.structSet(key, cfvariant(value.c_str()));
}

cfvariant *cf___configget(const cfvariant **args, int argc)
{
    (void)args;
    (void)argc;

    cfvariant root(cfvariant::Struct);

    cfvariant settings(cfvariant::Struct);
    putBool(settings, "enableWhitespaceManagement", webstrada::config::enableWhitespaceManagement);
    putString(settings, "defaultOutputCharset", webstrada::config::defaultOutputCharset);
    putString(settings, "defaultInputCharset", webstrada::config::defaultInputCharset);
    putInt(settings, "charsetDetectionMinConfidence", webstrada::config::charsetDetectionMinConfidence);
    putString(settings, "scopeDbPath", webstrada::config::scopeDbPath);
    putString(settings, "cacheDbPath", webstrada::config::cacheDbPath);
    putString(settings, "dsnDbDir", webstrada::config::dsnDbDir);
    putDouble(settings, "defaultApplicationTimeoutSeconds", webstrada::config::defaultApplicationTimeoutSeconds);
    putDouble(settings, "defaultSessionTimeoutSeconds", webstrada::config::defaultSessionTimeoutSeconds);
    putBool(settings, "enableQueryLogging", webstrada::config::enableQueryLogging);
    putBool(settings, "debugEnabled", webstrada::config::debugEnabled);
    putBool(settings, "lineExecutionTrace", webstrada::config::lineExecutionTrace);
    putString(settings, "compileExtForInclude", webstrada::config::compileExtForInclude);
    root.structSet("settings", settings);

    cfvariant dsRoot(cfvariant::Struct);
    for (const auto &kv : webstrada::config::datasources) {
        const webstrada::config::Datasource &ds = kv.second;
        cfvariant dsObj(cfvariant::Struct);
        putString(dsObj, "backend", ds.backend);
        putString(dsObj, "host", ds.host);
        putInt(dsObj, "port", ds.port);
        putString(dsObj, "database", ds.database);
        putString(dsObj, "username", ds.username);
        putString(dsObj, "password", ds.password.empty() ? "" : "****");
        dsRoot.structSet(kv.first.c_str(), dsObj);
    }
    root.structSet("datasources", dsRoot);

    return new cfvariant(root);
}

} // namespace cfml
