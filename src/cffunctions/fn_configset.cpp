/**
 * @file fn_configset.cpp
 * @brief Compiler-extension __configSet() built-in.
 *
 * Applies a partial configuration update to the server-wide config::* globals
 * and persists it to the config file. The argument is a struct shaped like
 * __configGet()'s result:
 *
 *     __configSet({settings: {enableQueryLogging: false},
 *                  datasources: {appdb: {backend: "mysql", host: "...", ...}}})
 *
 * Semantics:
 *   - Merge (not replace): only the keys present in the payload are touched.
 *   - Unknown setting keys throw (the engine lists the valid names).
 *   - Datasource passwords sent as the "****" sentinel keep the existing
 *     password (__configGet() masks real passwords with it).
 *   - A datasource whose struct carries action = "delete" is removed.
 *   - Named-argument calls are rejected (this is a positional-only extension).
 *
 * Returns __configGet()'s updated effective configuration.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/config.h>

#include <cstdlib>
#include <cstring>
#include <set>
#include <string>

namespace cfml {

cfvariant *cf___configget(const cfvariant **args, int argc);

namespace {

// The settings keys the engine understands. Keys not in this set are rejected.
static const std::set<std::string> &knownSettings()
{
    static const std::set<std::string> keys = {
        "enablewhitespacemanagement",
        "defaultoutputcharset",
        "defaultinputcharset",
        "charsetdetectionminconfidence",
        "scopedbpath",
        "cachedbpath",
        "dsndbdir",
        "defaultapplicationtimeoutseconds",
        "defaultsessiontimeoutseconds",
        "enablequerylogging",
        "debugenabled",
    };
    return keys;
}

static const std::set<std::string> &knownBackends()
{
    static const std::set<std::string> backends = {
        "sqlite", "mysql", "postgres", "postgresql", "pg",
    };
    return backends;
}

static std::string lower(const std::string &s)
{
    std::string out = s;
    for (auto &c : out) c = static_cast<char>(tolower((unsigned char)c));
    return out;
}

static void rejectNull(const cfvariant *v, const char *what)
{
    if (!v || v->m_type == cfvariant::Null) {
        throw webstrada::exception(webstrada::string("Compiler extension __configSet: ") + what + " must not be null.");
    }
}

static bool asBoolValue(const cfvariant *v, const char *what)
{
    rejectNull(v, what);
    return cfvariant_is_truthy(v);
}

static std::string asStringValue(const cfvariant *v, const char *what)
{
    rejectNull(v, what);
    return toStdString(v);
}

static double asDoubleValue(const cfvariant *v, const char *what)
{
    rejectNull(v, what);
    switch (v->m_type) {
        case cfvariant::Number:
        case cfvariant::Long:
        case cfvariant::Float:
            return getDoubleValue(*v);
        case cfvariant::Boolean:
            return v->m_bool ? 1.0 : 0.0;
        case cfvariant::String: {
            const char *p = v->m_str ? v->m_str->constData() : nullptr;
            if (!p || *p == '\0') break;
            char *end = nullptr;
            double d = strtod(p, &end);
            if (end != p && *end == '\0') return d;
            break;
        }
        default:
            break;
    }
    throw webstrada::exception(webstrada::string("Compiler extension __configSet: the '") + what + "' setting must be numeric.");
}

static int asIntValue(const cfvariant *v, const char *what)
{
    double d = asDoubleValue(v, what);
    if (d != static_cast<long long>(d) || d < -2147483648.0 || d > 2147483647.0) {
        throw webstrada::exception(webstrada::string("Compiler extension __configSet: the '") + what + "' setting must be an integer.");
    }
    return static_cast<int>(d);
}

// Applies a {key: value} settings struct onto the config::* globals.
static void applySettings(const cfvariant *settings)
{
    if (!settings) return;
    for (const auto &kv : *settings->m_struct) {
        const std::string key = lower(kv.first.constData() ? kv.first.constData() : "");
        const cfvariant *value = &kv.second;
        if (key == "enablewhitespacemanagement") {
            webstrada::config::enableWhitespaceManagement = asBoolValue(value, key.c_str());
        } else if (key == "defaultoutputcharset") {
            webstrada::config::defaultOutputCharset = asStringValue(value, key.c_str());
        } else if (key == "defaultinputcharset") {
            webstrada::config::defaultInputCharset = asStringValue(value, key.c_str());
        } else if (key == "charsetdetectionminconfidence") {
            webstrada::config::charsetDetectionMinConfidence = asIntValue(value, key.c_str());
        } else if (key == "scopedbpath") {
            webstrada::config::scopeDbPath = asStringValue(value, key.c_str());
        } else if (key == "cachedbpath") {
            webstrada::config::cacheDbPath = asStringValue(value, key.c_str());
        } else if (key == "dsndbdir") {
            webstrada::config::dsnDbDir = asStringValue(value, key.c_str());
        } else if (key == "defaultapplicationtimeoutseconds") {
            webstrada::config::defaultApplicationTimeoutSeconds = asDoubleValue(value, key.c_str());
        } else if (key == "defaultsessiontimeoutseconds") {
            webstrada::config::defaultSessionTimeoutSeconds = asDoubleValue(value, key.c_str());
        } else if (key == "enablequerylogging") {
            webstrada::config::enableQueryLogging = asBoolValue(value, key.c_str());
        } else if (key == "debugenabled") {
            webstrada::config::debugEnabled = asBoolValue(value, key.c_str());
        } else {
            std::string valid;
            for (const auto &k : knownSettings()) {
                if (!valid.empty()) valid += ", ";
                valid += k;
            }
            throw webstrada::exception(webstrada::string("Compiler extension __configSet: unknown setting '") +
                                       key.c_str() + "'. Valid settings: " + valid.c_str() + ".");
        }
    }
}

// Applies a {name: {...}} datasources struct (merge semantics; action=delete
// removes a datasource). The name keeps its entered casing (the map's CiStrLess
// comparator keeps lookups case-insensitive).
static void applyDatasources(const cfvariant *dsRoot)
{
    if (!dsRoot) return;
    for (const auto &kv : *dsRoot->m_struct) {
        std::string name = kv.first.constData() ? kv.first.constData() : "";

        const cfvariant *dsVal = &kv.second;
        rejectNull(dsVal, ("the datasource '" + name + "'").c_str());
        if (dsVal->m_type != cfvariant::Struct) {
            throw webstrada::exception(webstrada::string("Compiler extension __configSet: the datasource '") +
                                       name.c_str() + "' value must be a struct.");
        }

        // action = "delete" removes the datasource.
        const cfvariant *action = structGet(dsVal, "action");
        if (action && lower(toStdString(action)) == "delete") {
            auto it = webstrada::config::datasources.find(name);
            if (it == webstrada::config::datasources.end()) {
                throw webstrada::exception(webstrada::string("Compiler extension __configSet: datasource '") +
                                           name.c_str() + "' is not configured.");
            }
            webstrada::config::datasources.erase(it);
            continue;
        }

        webstrada::config::Datasource ds;
        const cfvariant *backend = structGet(dsVal, "backend");
        if (backend) {
            ds.backend = lower(toStdString(backend));
            if (knownBackends().find(ds.backend) == knownBackends().end()) {
                throw webstrada::exception(webstrada::string("Compiler extension __configSet: datasource '") +
                                           name.c_str() + "' has unsupported backend '" + ds.backend.c_str() +
                                           "'. Valid backends: sqlite, mysql, postgres, postgresql, pg.");
            }
        }
        const cfvariant *host = structGet(dsVal, "host");
        if (host) ds.host = asStringValue(host, "host");
        const cfvariant *port = structGet(dsVal, "port");
        if (port) ds.port = asIntValue(port, "port");
        const cfvariant *database = structGet(dsVal, "database");
        if (database) ds.database = asStringValue(database, "database");
        const cfvariant *username = structGet(dsVal, "username");
        if (username) ds.username = asStringValue(username, "username");

        const cfvariant *password = structGet(dsVal, "password");
        if (password) {
            std::string pw = asStringValue(password, "password");
            if (pw == "****") {
                // Keep the existing password: __configGet() masks real passwords
                // with this sentinel, so an untouched round-trip must not
                // clobber the stored secret.
                auto it = webstrada::config::datasources.find(name);
                if (it != webstrada::config::datasources.end()) {
                    ds.password = it->second.password;
                }
            } else {
                ds.password = pw;
            }
        }

        webstrada::config::datasources[name] = std::move(ds);
    }
}

} // namespace

cfvariant *cf___configset(const cfvariant **args, int argc)
{
    if (argc < 1 || !args || !args[0]) {
        throw webstrada::exception("Compiler extension __configSet requires exactly 1 argument.");
    }
    const cfvariant *arg = args[0];
    if (arg->m_type == cfvariant::Struct) {
        // A named-arguments marker (name = value) is rejected: the extension
        // family is positional-only.
        if (arg->m_struct->find(CFML_NAMED_ARGS_KEY) != arg->m_struct->end()) {
            throw webstrada::exception(
                "Compiler extension __configSet does not accept named arguments.");
        }
    } else {
        throw webstrada::exception(
            "Compiler extension __configSet requires a struct argument "
            "({settings: {...}, datasources: {...}}).");
    }

    const cfvariant *settings = structGet(arg, "settings");
    const cfvariant *dsRoot = structGet(arg, "datasources");
    if (!settings && !dsRoot) {
        throw webstrada::exception(
            "Compiler extension __configSet: the argument must contain a "
            "'settings' and/or 'datasources' member.");
    }

    applySettings(settings);
    applyDatasources(dsRoot);

    // Persist (a no-op for processes without a config file, e.g. the CLI).
    webstrada::config::save();

    // Return the updated effective configuration.
    return cf___configget(nullptr, 0);
}

} // namespace cfml
