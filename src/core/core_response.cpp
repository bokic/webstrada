#include "core_internal.h"
#include "../cftags/common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <webstrada/parser.h>
#include <webstrada/worker.h>
#include <webstrada/cfimage.h>
#include <webstrada/cfvariant.h>
#include <webstrada/string.h>
#include <webstrada/scope_store.h>
#include <webstrada/config.h>
#include <webstrada/locale.h>
#include <webstrada/cfimage.h>

#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>
#include <sqlite3.h>
#include <curl/curl.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/hmac.h>
#include <openssl/provider.h>

#include <thread>
#include <chrono>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <ctime>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <fstream>
#include <filesystem>
#include <unistd.h>
#include <fcntl.h>

using namespace webstrada;
using namespace cfml;
// ---- output / response state ----
// ---------------------------------------------------------------------------

void cfml::cfwriteoutput(webstrada::string &out, const char *text, size_t size)
{
    if (cfml::response().binary) return; // cfcontent file/variable: other output ignored
    if (size > 0) {
        out.append(text, size);
    }
}

void cfml::cfoutputvariant(webstrada::string &out, const cfvariant *value)
{
    if (cfml::response().binary || !value) return;
    out.append(const_cast<cfvariant *>(value)->toString());
}

void cfml::cf_whitespace_space(webstrada::string &out)
{
    if (cfml::response().binary) return; // cfcontent file/variable: other output ignored
    if (out.isEmpty()) return;
    char c = out.at(out.length() - 1);
    if (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v') return;
    out.append(' ');
}

// Raw HTTP request body of the current request, captured by the daemon worker
// (see request_set_body). GetHttpRequestData reads it for the `content` key;
// the CLI has none (empty).
thread_local std::string g_requestBody;

// ---- HTTP response output state (<cfcontent> / <cfflush>) ----

namespace webstrada {
std::string config::defaultOutputCharset = "UTF-8";
std::string config::defaultInputCharset = "UTF-8";
int config::charsetDetectionMinConfidence = 80;
std::string config::scopeDbPath;
std::string config::cacheDbPath;
std::string config::dsnDbDir;
std::map<std::string, config::Datasource, config::CiStrLess> config::datasources;
double config::defaultApplicationTimeoutSeconds = 2.0 * 24.0 * 3600.0;   // CF Admin default: 2 days
double config::defaultSessionTimeoutSeconds = 20.0 * 60.0;                // CF Admin default: 20 minutes
bool config::enableQueryLogging = true;

// Whether the CF Administrator "Enable Debugging" setting is on. This engine
// has no debug output section, so it stays false (the CF default and the RDS
// host setting); <cftimer>'s inline/comment/outline timing display and the
// <cftrace> tag are gated on it exactly like CF's DebuggingService.
bool config::debugEnabled = false;

// CF Administrator "compile extensions for include" (`compileextforinclude`).
// ColdFusion's stock install ships `*` (see payload/core/WEB-INF/cfusion/lib/
// neo-runtime.xml), and the RDS host keeps that default: every file included
// via <cfinclude> is compiled and executed as CFML, not just .cfm/.cfml. This
// is what lets MangoBlog's setup run its `<cfinclude template="mysql.sql">`
// DDL script. An empty value reproduces the old engine behavior (only .cfm/
// .cfml compile; other extensions are read and output raw).
std::string config::compileExtForInclude = "*";

void config::loadDatasourcesFromEnv()
{
    extern char **environ;
    // Resolve the global environ (not webstrada::config::environ).
    char **env = ::environ;
    static const char kPrefix[] = "WSDATASOURCE_";
    static const size_t kPrefixLen = sizeof(kPrefix) - 1;
    for (char **e = env; e && *e; e++) {
        std::string var(*e);
        if (var.rfind(kPrefix, 0) != 0) continue;
        size_t eq = var.find('=');
        std::string name = var.substr(kPrefixLen, eq == std::string::npos ? std::string::npos : eq - kPrefixLen);
        std::string value = (eq == std::string::npos) ? "" : var.substr(eq + 1);
        if (name.empty()) continue;

        // The datasource name is the variable name up to the first '_' after
        // the prefix. The DSN in CFML is the UPPERCASED name with '-' replaced
        // by '_'.
        size_t sep = name.find('_');
        std::string dsnKey = (sep == std::string::npos) ? name : name.substr(0, sep);
        std::string field = (sep == std::string::npos) ? "" : name.substr(sep + 1);
        if (dsnKey.empty()) continue;

        // Lowercase the field for comparison; keep the value as-is.
        std::string fieldLow;
        for (char c : field) fieldLow += static_cast<char>(tolower((unsigned char)c));

        config::Datasource &ds = config::datasources[dsnKey];
        if (fieldLow == "backend") ds.backend = value;
        else if (fieldLow == "host") ds.host = value;
        else if (fieldLow == "port") ds.port = atoi(value.c_str());
        else if (fieldLow == "database") ds.database = value;
        else if (fieldLow == "username") ds.username = value;
        else if (fieldLow == "password") ds.password = value;
    }
}

} // namespace webstrada



// Current unix-epoch seconds (from the system clock).
int64_t nowSeconds()
{
    return static_cast<int64_t>(std::time(nullptr));
}
