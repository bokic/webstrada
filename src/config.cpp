// Server configuration file support.
//
// The server configuration lives in a JSON file (webstrada-config.json) that
// the daemon loads into the config::* globals at startup (see
// webstrada/config.h). The admin panel reads and writes it through the
// __configGet()/__configSet() compiler-extension functions. This file also
// tracks the file's mtime so prefork workers can re-read it per request
// (config::reloadIfChanged) and pick up admin-panel writes without a restart.

#include <webstrada/config.h>

#include <json-c/json.h>

#include <filesystem>
#include <fstream>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <vector>
#include <unistd.h>

namespace fs = std::filesystem;

namespace webstrada {
namespace config {

std::string configFilePath;
bool lineExecutionTrace = false;

static void (*s_cacheInvalidator)() = nullptr;

void setCacheInvalidator(void (*fn)())
{
    s_cacheInvalidator = fn;
}

void invalidateCompiledCaches()
{
    if (s_cacheInvalidator) s_cacheInvalidator();
}

static fs::file_time_type s_lastLoadMtime{};
static bool s_loaded = false;

// Resolves the default config path: <binary-dir>/webstrada-config.json, the
// same rule the SQLite scope/cache databases use (worker.cpp).
static std::string defaultConfigPath()
{
    char exe[4096];
    ssize_t n = readlink("/proc/self/exe", exe, sizeof(exe) - 1);
    std::string path;
    if (n > 0) {
        exe[n] = '\0';
        std::string p(exe);
        size_t slash = p.find_last_of('/');
        path = (slash != std::string::npos)
            ? p.substr(0, slash + 1) + "webstrada-config.json"
            : "webstrada-config.json";
    } else {
        path = "webstrada-config.json";
    }
    return path;
}

// Reads a json-c object value into the named JSON types.
static std::string jsonStringField(json_object *obj, const char *key, const std::string &fallback)
{
    json_object *v = nullptr;
    if (obj && json_object_object_get_ex(obj, key, &v) && json_object_is_type(v, json_type_string)) {
        return json_object_get_string(v);
    }
    return fallback;
}

static bool jsonBoolField(json_object *obj, const char *key, bool fallback)
{
    json_object *v = nullptr;
    if (obj && json_object_object_get_ex(obj, key, &v) && json_object_is_type(v, json_type_boolean)) {
        return json_object_get_boolean(v) != 0;
    }
    return fallback;
}

static double jsonDoubleField(json_object *obj, const char *key, double fallback)
{
    json_object *v = nullptr;
    if (obj && json_object_object_get_ex(obj, key, &v) &&
        (json_object_is_type(v, json_type_double) || json_object_is_type(v, json_type_int))) {
        return json_object_get_double(v);
    }
    return fallback;
}

static int jsonIntField(json_object *obj, const char *key, int fallback)
{
    json_object *v = nullptr;
    if (obj && json_object_object_get_ex(obj, key, &v) &&
        (json_object_is_type(v, json_type_double) || json_object_is_type(v, json_type_int))) {
        return json_object_get_int(v);
    }
    return fallback;
}

// Applies a parsed JSON root ({settings: {...}, datasources: {...}}) onto the
// config::* globals. Unknown keys are ignored (the file may carry keys this
// engine version does not know). Returns false on a structural error.
static bool applyFile(json_object *root)
{
    if (!root || !json_object_is_type(root, json_type_object)) return false;

    json_object *settings = nullptr;
    if (json_object_object_get_ex(root, "settings", &settings) &&
        json_object_is_type(settings, json_type_object)) {
        enableWhitespaceManagement   = jsonBoolField(settings, "enableWhitespaceManagement", enableWhitespaceManagement);
        defaultOutputCharset         = jsonStringField(settings, "defaultOutputCharset", defaultOutputCharset);
        defaultInputCharset          = jsonStringField(settings, "defaultInputCharset", defaultInputCharset);
        charsetDetectionMinConfidence = jsonIntField(settings, "charsetDetectionMinConfidence", charsetDetectionMinConfidence);
        scopeDbPath                  = jsonStringField(settings, "scopeDbPath", scopeDbPath);
        cacheDbPath                  = jsonStringField(settings, "cacheDbPath", cacheDbPath);
        dsnDbDir                     = jsonStringField(settings, "dsnDbDir", dsnDbDir);
        defaultApplicationTimeoutSeconds = jsonDoubleField(settings, "defaultApplicationTimeoutSeconds", defaultApplicationTimeoutSeconds);
        defaultSessionTimeoutSeconds = jsonDoubleField(settings, "defaultSessionTimeoutSeconds", defaultSessionTimeoutSeconds);
        enableQueryLogging           = jsonBoolField(settings, "enableQueryLogging", enableQueryLogging);
        debugEnabled                 = jsonBoolField(settings, "debugEnabled", debugEnabled);
        bool prevTrace               = lineExecutionTrace;
        lineExecutionTrace           = jsonBoolField(settings, "lineExecutionTrace", lineExecutionTrace);
        if (lineExecutionTrace != prevTrace) {
            invalidateCompiledCaches();
        }
        compileExtForInclude         = jsonStringField(settings, "compileExtForInclude", compileExtForInclude);
    }

    json_object *dsObjs = nullptr;
    if (json_object_object_get_ex(root, "datasources", &dsObjs) &&
        json_object_is_type(dsObjs, json_type_object)) {
        datasources.clear();
        json_object_object_foreach(dsObjs, dsnKey, dsVal) {
            if (!json_object_is_type(dsVal, json_type_object)) continue;
            Datasource ds;
            ds.backend  = jsonStringField(dsVal, "backend", "sqlite");
            ds.host     = jsonStringField(dsVal, "host", "");
            ds.port     = jsonIntField(dsVal, "port", 0);
            ds.database = jsonStringField(dsVal, "database", "");
            ds.username = jsonStringField(dsVal, "username", "");
            ds.password = jsonStringField(dsVal, "password", "");
            datasources[dsnKey] = std::move(ds);
        }
    }
    return true;
}

static void loadFile()
{
    fs::path path(configFilePath);
    std::error_code ec;
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        fprintf(stderr, "[WebStrada] Warning: could not open config file %s\n", configFilePath.c_str());
        s_loaded = true;
        return;
    }
    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    json_object *root = json_tokener_parse(content.c_str());
    if (!root) {
        fprintf(stderr, "[WebStrada] Warning: invalid JSON in config file %s, keeping current settings\n",
                configFilePath.c_str());
        s_loaded = true;
        return;
    }
    applyFile(root);
    json_object_put(root);
    s_lastLoadMtime = fs::last_write_time(path, ec);
    s_loaded = true;
}

void initialize()
{
    if (configFilePath.empty()) {
        const char *env = getenv("WEBSTRADA_CONFIG");
        configFilePath = (env && *env) ? env : defaultConfigPath();
    }
    if (!fs::exists(configFilePath)) {
        // Bootstrap: write the current defaults out so the file always
        // reflects the effective configuration.
        save();
    } else {
        loadFile();
    }
}

bool hasConfigFile()
{
    return !configFilePath.empty() && fs::exists(configFilePath);
}

bool reloadIfChanged()
{
    if (!s_loaded || configFilePath.empty()) return false;
    std::error_code ec;
    auto mtime = fs::last_write_time(configFilePath, ec);
    if (ec || mtime == s_lastLoadMtime) return false;
    loadFile();
    return true;
}

// Serializes the current config::* globals into a json_object tree.
static json_object *globalsToJson()
{
    json_object *root = json_object_new_object();

    json_object *settings = json_object_new_object();
    json_object_object_add(settings, "enableWhitespaceManagement", json_object_new_boolean(enableWhitespaceManagement));
    json_object_object_add(settings, "defaultOutputCharset", json_object_new_string(defaultOutputCharset.c_str()));
    json_object_object_add(settings, "defaultInputCharset", json_object_new_string(defaultInputCharset.c_str()));
    json_object_object_add(settings, "charsetDetectionMinConfidence", json_object_new_int(charsetDetectionMinConfidence));
    json_object_object_add(settings, "scopeDbPath", json_object_new_string(scopeDbPath.c_str()));
    json_object_object_add(settings, "cacheDbPath", json_object_new_string(cacheDbPath.c_str()));
    json_object_object_add(settings, "dsnDbDir", json_object_new_string(dsnDbDir.c_str()));
    json_object_object_add(settings, "defaultApplicationTimeoutSeconds", json_object_new_double(defaultApplicationTimeoutSeconds));
    json_object_object_add(settings, "defaultSessionTimeoutSeconds", json_object_new_double(defaultSessionTimeoutSeconds));
    json_object_object_add(settings, "enableQueryLogging", json_object_new_boolean(enableQueryLogging));
    json_object_object_add(settings, "debugEnabled", json_object_new_boolean(debugEnabled));
    json_object_object_add(settings, "lineExecutionTrace", json_object_new_boolean(lineExecutionTrace));
    json_object_object_add(settings, "compileExtForInclude", json_object_new_string(compileExtForInclude.c_str()));
    json_object_object_add(root, "settings", settings);

    json_object *dsObjs = json_object_new_object();
    for (const auto &kv : datasources) {
        const Datasource &ds = kv.second;
        json_object *dsObj = json_object_new_object();
        json_object_object_add(dsObj, "backend", json_object_new_string(ds.backend.c_str()));
        json_object_object_add(dsObj, "host", json_object_new_string(ds.host.c_str()));
        json_object_object_add(dsObj, "port", json_object_new_int(ds.port));
        json_object_object_add(dsObj, "database", json_object_new_string(ds.database.c_str()));
        json_object_object_add(dsObj, "username", json_object_new_string(ds.username.c_str()));
        json_object_object_add(dsObj, "password", json_object_new_string(ds.password.c_str()));
        json_object_object_add(dsObjs, kv.first.c_str(), dsObj);
    }
    json_object_object_add(root, "datasources", dsObjs);
    return root;
}

void save()
{
    if (configFilePath.empty()) return;
    json_object *root = globalsToJson();
    std::string text(json_object_to_json_string_ext(root, JSON_C_TO_STRING_PRETTY | JSON_C_TO_STRING_NOSLASHESCAPE));
    json_object_put(root);
    // Atomic write: a temp file in the same directory, then rename. A torn
    // admin-panel write can never leave a half-written config file behind.
    std::string tmpPath = configFilePath + ".tmp";
    {
        std::ofstream out(tmpPath, std::ios::binary | std::ios::trunc);
        if (!out) {
            fprintf(stderr, "[WebStrada] Warning: could not write config file %s\n", tmpPath.c_str());
            return;
        }
        out.write(text.data(), static_cast<std::streamsize>(text.size()));
        out.flush();
        if (!out) {
            fprintf(stderr, "[WebStrada] Warning: failed writing config file %s\n", tmpPath.c_str());
            return;
        }
    }
    std::error_code ec;
    fs::rename(tmpPath, configFilePath, ec);
    if (ec) {
        fprintf(stderr, "[WebStrada] Warning: could not replace config file %s: %s\n",
                configFilePath.c_str(), ec.message().c_str());
        return;
    }
    s_lastLoadMtime = fs::last_write_time(configFilePath, ec);
    s_loaded = true;
}

void resetToDefaults()
{
    // The built-in defaults (mirrors the static initializers in
    // core_response.cpp / llvm_codegen.cpp; also CF's Administrator defaults).
    enableWhitespaceManagement = true;
    defaultOutputCharset = "UTF-8";
    defaultInputCharset = "UTF-8";
    charsetDetectionMinConfidence = 80;
    scopeDbPath.clear();
    cacheDbPath.clear();
    dsnDbDir.clear();
    datasources.clear();
    defaultApplicationTimeoutSeconds = 2.0 * 24.0 * 3600.0;   // 2 days
    defaultSessionTimeoutSeconds = 20.0 * 60.0;                // 20 minutes
    enableQueryLogging = true;
    debugEnabled = false;
    lineExecutionTrace = false;
    invalidateCompiledCaches();
    compileExtForInclude = "*";
    save();
}

} // namespace config
} // namespace webstrada
