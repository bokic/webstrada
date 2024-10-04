/**
 * @file tag_cache.cpp
 * @brief <cfcache> runtime (template/fragment caching, client-cache headers,
 *        flush, object-cache put/get).
 *
 * Mirrors coldfusion.tagext.io.cache.CacheTag / CacheTagHelper / CachingFilter:
 *  - the body form (<cfcache>...</cfcache>) stores the rendered fragment in the
 *    TEMPLATE cache region keyed by page path + line; on a hit the cached
 *    fragment is written and the body is skipped;
 *  - the self-closing form (whole page) registers a pending store that the
 *    request harness finalizes via cf_cache_store_page after the page
 *    completes; on a hit the cached page (status, type, headers, cookies,
 *    body) is restored and the rest of the page is skipped;
 *  - action="clientcache" adds Expires/Cache-Control/Pragma headers;
 *  - action="flush" removes template-cache entries matching expireURL (or all,
 *    when neither id nor expireurl is given) or object-cache entries by id;
 *  - action="put"/"get" store/read object-cache values (SerializeJSON blobs).
 *
 * The engine's cache store is SQLite-backed, so the `directory` attribute is
 * validated like CF (empty / non-existent) but does not redirect storage.
 */

#include "common.h"

#include "../cftags/common.h"
#include <webstrada/cf8.h>
#include <webstrada/cache_store.h>
#include <webstrada/cfvariant.h>
#include <webstrada/config.h>
#include <webstrada/exceptions.h>
#include <webstrada/string.h>

#include <json-c/json.h>

#include <openssl/evp.h>

#include <cctype>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <regex>
#include <sys/stat.h>
#include <filesystem>
#include <string>
#include <vector>

namespace cfml {

namespace {

// ---- attribute helpers ---------------------------------------------------

std::string variantStr(const cfvariant *v)
{
    if (!v) return std::string();
    webstrada::string s = const_cast<cfvariant*>(v)->toString();
    return s.constData() ? std::string(s.constData(), s.length()) : std::string();
}

// Trim whitespace and lowercase (used for action/protocol/region comparisons).
std::string trimmedLower(const std::string &s)
{
    size_t b = 0, e = s.size();
    while (b < e && (unsigned char)s[b] <= 0x20) b++;
    while (e > b && (unsigned char)s[e-1] <= 0x20) e--;
    std::string t = s.substr(b, e - b);
    for (char &c : t) c = (char)tolower((unsigned char)c);
    return t;
}

bool boolAttr(const cfvariant *v, bool def)
{
    return v ? cfmlBoolean(v, def) : def;
}

// CF's setTimespan/setIdleTime: days -> seconds. Returns -1 when the attribute
// is absent (CF's field default; -1 maps to the cache config default TTL).
int64_t secondsFromDays(const cfvariant *v)
{
    if (!v || v->m_type == cfvariant::Null) return -1;
    double days = getDoubleValue(*v);
    if (days < 0) return -1;
    return (int64_t)(days * 86400.0);
}

// Resolve a raw seconds interval (CF's -1 = "not specified") into milliseconds:
// negative uses CF's default cache config (1 day); 0 stays 0 (both 0 = eternal).
int64_t resolveIntervalMs(int64_t sec)
{
    return sec < 0 ? 86400LL * 1000LL : sec * 1000LL;
}

// Region selection: the `region` attribute wins, then `key`, then the default.
std::string resolveRegion(const cfvariant *region, const cfvariant *key,
                          const char *defRegion)
{
    if (region) {
        std::string r = trimmedLower(variantStr(region));
        if (!r.empty()) {
            if (r == "object") return "OBJECT";
            if (r == "query") return "QUERY";
            if (r == "template") return "TEMPLATE";
            return r;
        }
    }
    if (key) {
        std::string r = trimmedLower(variantStr(key));
        if (!r.empty()) {
            if (r == "object") return "OBJECT";
            if (r == "query") return "QUERY";
            if (r == "template") return "TEMPLATE";
            return r;
        }
    }
    return defRegion;
}

// ---- crypto / date helpers ----------------------------------------------

std::string sha256Hex(const std::string &input)
{
    std::vector<unsigned char> digest(static_cast<size_t>(EVP_MAX_MD_SIZE));
    unsigned int digestLen = 0;
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    if (!ctx) return "";
    int rc = EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr);
    if (rc == 1) rc = EVP_DigestUpdate(ctx, input.data(), input.size());
    if (rc == 1) rc = EVP_DigestFinal_ex(ctx, digest.data(), &digestLen);
    EVP_MD_CTX_free(ctx);
    if (rc != 1) return "";
    char buf[EVP_MAX_MD_SIZE * 2 + 1];
    for (unsigned int i = 0; i < digestLen; i++) {
        std::sprintf(buf + i * 2, "%02x", digest[i]);
    }
    return std::string(buf);
}

// CF's httpDateFormatter: "EEE, dd MMM yyyy HH:mm:ss zz" in GMT/English.
std::string httpDateString(int64_t unixSeconds)
{
    time_t t = static_cast<time_t>(unixSeconds);
    struct tm tm_utc;
    memset(&tm_utc, 0, sizeof(tm_utc));
    gmtime_r(&t, &tm_utc);
    static const char *wd[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    static const char *mn[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul",
                               "Aug", "Sep", "Oct", "Nov", "Dec"};
    char buf[128];
    std::sprintf(buf, "%s, %02d %s %04d %02d:%02d:%02d GMT",
                 wd[tm_utc.tm_wday], tm_utc.tm_mday, mn[tm_utc.tm_mon],
                 tm_utc.tm_year + 1900, tm_utc.tm_hour, tm_utc.tm_min, tm_utc.tm_sec);
    return std::string(buf);
}

// CF's CacheTagHelper.stripWhiteSpaces:
// REReplace(body, "[\n\r\t]{2,}", SecurityManager.tokenSeparator, "ALL", true)
// where tokenSeparator is a carriage return ("\r"), not a space.
std::string stripWhitespaces(const std::string &body)
{
    std::string out;
    out.reserve(body.size());
    int run = 0;
    for (char c : body) {
        if (c == '\n' || c == '\r' || c == '\t') {
            run++;
        } else {
            if (run >= 2) out.push_back('\r');
            run = 0;
            out.push_back(c);
        }
    }
    if (run >= 2) out.push_back('\r');
    return out;
}

// ---- serialization -------------------------------------------------------

std::string serializeFragment(const std::string &body)
{
    json_object *obj = json_object_new_object();
    json_object_object_add(obj, "__CACHE_FRAGMENT__", json_object_new_boolean(1));
    json_object_object_add(obj, "body", json_object_new_string_len(body.data(), (int)body.size()));
    const char *s = json_object_to_json_string_ext(obj, JSON_C_TO_STRING_PLAIN | JSON_C_TO_STRING_NOSLASHESCAPE);
    std::string ret = s ? s : "";
    json_object_put(obj);
    return ret;
}

bool deserializeFragment(const std::string &blob, std::string &body)
{
    json_object *obj = json_tokener_parse(blob.c_str());
    if (!obj) return false;
    bool ok = false;
    json_object *b = nullptr;
    if (json_object_object_get_ex(obj, "body", &b) && json_object_get_type(b) == json_type_string) {
        body.assign(json_object_get_string(b), (size_t)json_object_get_string_len(b));
        ok = true;
    }
    json_object_put(obj);
    return ok;
}

// Serialize the whole-page response (status, type, charset, headers, cookies,
// body) into a JSON marker stored in the TEMPLATE region.
std::string serializePage(const response_state &r, const std::string &body)
{
    json_object *obj = json_object_new_object();
    json_object_object_add(obj, "__CACHE_PAGE__", json_object_new_boolean(1));
    json_object_object_add(obj, "status", json_object_new_int(r.statusCode));
    json_object_object_add(obj, "type", json_object_new_string(r.contentType.constData() ? r.contentType.constData() : ""));
    json_object_object_add(obj, "charset", json_object_new_string(r.charset.constData() ? r.charset.constData() : ""));
    json_object *h = json_object_new_array();
    for (const auto &p : r.headers) {
        json_object *hp = json_object_new_array();
        json_object_array_add(hp, json_object_new_string(p.first.c_str()));
        json_object_array_add(hp, json_object_new_string(p.second.c_str()));
        json_object_array_add(h, hp);
    }
    json_object_object_add(obj, "headers", h);
    json_object *ck = json_object_new_array();
    for (const auto &c : r.cookies) {
        json_object_array_add(ck, json_object_new_string(c.c_str()));
    }
    json_object_object_add(obj, "cookies", ck);
    json_object_object_add(obj, "body", json_object_new_string_len(body.data(), (int)body.size()));
    const char *s = json_object_to_json_string_ext(obj, JSON_C_TO_STRING_PLAIN | JSON_C_TO_STRING_NOSLASHESCAPE);
    std::string ret = s ? s : "";
    json_object_put(obj);
    return ret;
}

// Restore a whole-page blob into the response state; the body is returned
// separately (the caller decides where to write it). Returns false when the
// blob is not a whole-page entry.
bool deserializePage(const std::string &blob, response_state &r, std::string &body)
{
    json_object *obj = json_tokener_parse(blob.c_str());
    if (!obj) return false;
    bool ok = false;
    json_object *page = nullptr;
    if (json_object_object_get_ex(obj, "__CACHE_PAGE__", &page) && json_object_get_boolean(page)) {
        json_object *st = nullptr;
        if (json_object_object_get_ex(obj, "status", &st) && json_object_get_type(st) == json_type_int) {
            r.statusCode = json_object_get_int(st);
        }
        json_object *t = nullptr;
        if (json_object_object_get_ex(obj, "type", &t) && json_object_get_type(t) == json_type_string) {
            r.contentType = json_object_get_string(t);
        }
        json_object *cs = nullptr;
        if (json_object_object_get_ex(obj, "charset", &cs) && json_object_get_type(cs) == json_type_string) {
            r.charset = json_object_get_string(cs);
        }
        r.headers.clear();
        json_object *hdrs = nullptr;
        if (json_object_object_get_ex(obj, "headers", &hdrs) && json_object_get_type(hdrs) == json_type_array) {
            size_t n = json_object_array_length(hdrs);
            for (size_t i = 0; i < n; i++) {
                json_object *hp = json_object_array_get_idx(hdrs, i);
                if (json_object_get_type(hp) != json_type_array) continue;
                size_t m = json_object_array_length(hp);
                if (m < 1) continue;
                json_object *hn = json_object_array_get_idx(hp, 0);
                if (json_object_get_type(hn) != json_type_string) continue;
                std::string hname = json_object_get_string(hn);
                std::string hval;
                if (m > 1) {
                    json_object *hv = json_object_array_get_idx(hp, 1);
                    if (json_object_get_type(hv) == json_type_string) hval = json_object_get_string(hv);
                }
                r.headers.emplace_back(hname, hval);
            }
        }
        r.cookies.clear();
        json_object *cks = nullptr;
        if (json_object_object_get_ex(obj, "cookies", &cks) && json_object_get_type(cks) == json_type_array) {
            size_t n = json_object_array_length(cks);
            for (size_t i = 0; i < n; i++) {
                json_object *c = json_object_array_get_idx(cks, i);
                if (json_object_get_type(c) == json_type_string) {
                    r.cookies.emplace_back(json_object_get_string(c));
                }
            }
        }
        body.clear();
        json_object *b = nullptr;
        if (json_object_object_get_ex(obj, "body", &b) && json_object_get_type(b) == json_type_string) {
            body.assign(json_object_get_string(b), (size_t)json_object_get_string_len(b));
        }
        ok = true;
    }
    json_object_put(obj);
    return ok;
}

// ---- thread-local per-request state --------------------------------------

// A pending whole-page store, registered by a self-closing <cfcache> miss and
// finalized by the request harness via cf_cache_store_page.
struct PageCacheCfg {
    bool active = false;
    std::string key;
    std::string region;
    int64_t ttlMs = 0;
    int64_t idleMs = 0;
    bool stripWhitespace = false;
};
thread_local PageCacheCfg g_pageCacheCfg;

// A pending fragment store for the body form (<cfcache>...</cfcache>), pushed
// by begin on the miss path and finalized by the compiled end tag.
struct FragCapture {
    size_t startLen = 0;
    std::string region;
    std::string key;
    bool stripWhitespace = false;
    int64_t ttlMs = 0;
    int64_t idleMs = 0;
    std::string metadataName;  // the `metadata` variable to set after the store
};
thread_local std::vector<FragCapture> g_fragCaptures;

// ---- cache helpers -------------------------------------------------------

webstrada::CacheStore &store()
{
    return webstrada::cache_store();
}

bool cacheOpen()
{
    return store().isOpen();
}

int64_t nowMs()
{
    return static_cast<int64_t>(::time(nullptr)) * 1000;
}

// The current page's filesystem path (the request's template), used to build
// the template cache key like CF's fContext.getPagePath().
std::string currentPagePath()
{
    cfml::IncludeRuntime *rt = cfml::include_context();
    if (rt && !rt->currentPath.empty()) return rt->currentPath;
    return "";
}

// Query string of the current request (lowercased when folded into the key).
std::string currentQueryString(void *cgi)
{
    if (!cgi) return "";
    cfvariant *c = static_cast<cfvariant*>(cgi);
    if (c->m_type != cfvariant::Struct || !c->m_struct) return "";
    auto it = c->m_struct->find("QUERY_STRING");
    if (it == c->m_struct->end()) return "";
    webstrada::string s = it->second.toString();
    return s.constData() ? std::string(s.constData(), s.length()) : "";
}

// Build the template-cache key (CacheTag.generateTemplateKey). The `id`
// attribute does NOT take part: CF's generateTemplateKey only uses `this.id`
// (the JSP TagSupport id, which the cfcache setId override never sets), so the
// key is always request-page + optional _query_ + _pageid:<sha256> + (_line:N
// for the body form) + dependsOn values. The `id` attribute is used only by the
// object-cache actions (put/get/flush).
std::string templateCacheKey(bool useURL, void *cgi, bool hasEndTag, int lineNo,
                             const std::vector<std::string> &dependsOn)
{
    std::string key = currentPagePath();
    if (useURL) {
        std::string qs = currentQueryString(cgi);
        for (char &c : qs) c = (char)tolower((unsigned char)c);
        key += "_query_" + qs;
    }
    key += "_pageid:" + sha256Hex(currentPagePath());
    if (hasEndTag) {
        key += "_line:" + std::to_string(lineNo);
    }
    for (const auto &d : dependsOn) {
        key += d;
    }
    return key;
}

// CF's setDependsOn: split on commas; each entry becomes "name" or
// "name=value" (when the variable resolves); an empty entry is an error.
std::vector<std::string> parseDependsOn(const cfvariant *dependson, void *variables)
{
    std::vector<std::string> out;
    if (!dependson) return out;
    std::string str = variantStr(dependson);
    size_t start = 0;
    while (start <= str.size()) {
        size_t comma = str.find(',', start);
        std::string item = str.substr(start, comma == std::string::npos ? std::string::npos : comma - start);
        size_t b = 0, e = item.size();
        while (b < e && (unsigned char)item[b] <= 0x20) b++;
        while (e > b && (unsigned char)item[e-1] <= 0x20) e--;
        std::string dep = item.substr(b, e - b);
        if (dep.empty()) {
            throw webstrada::exception(webstrada::string("Application"),
                webstrada::string("The attribute dependson specified in the cfcache tag is either empty or invalid."),
                webstrada::string(""));
        }
        if (variables) {
            cfvariant *v = static_cast<cfvariant*>(variables);
            if (v->m_type == cfvariant::Struct && v->m_struct) {
                auto it = v->m_struct->find(dep.c_str());
                if (it != v->m_struct->end()) {
                    dep += "=" + variantStr(&it->second);
                }
            }
        }
        out.push_back(dep);
        if (comma == std::string::npos) break;
        start = comma + 1;
    }
    return out;
}

// CF's CacheTag.setDirectory validation: non-empty, and must resolve to an
// existing directory. The resolved directory is validated but storage stays in
// the engine's SQLite cache (a documented divergence).
void validateDirectory(const cfvariant *directory)
{
    if (!directory) return;
    std::string dir = variantStr(directory);
    size_t b = 0, e = dir.size();
    while (b < e && (unsigned char)dir[b] <= 0x20) b++;
    while (e > b && (unsigned char)dir[e-1] <= 0x20) e--;
    dir = dir.substr(b, e - b);
    if (dir.empty()) {
        throw webstrada::exception(webstrada::string("Application"),
            webstrada::string("The attribute directory specified in the cfcache tag is either empty or invalid."),
            webstrada::string(""));
    }
    if (!std::filesystem::is_directory(std::filesystem::path(dir))) {
        throw webstrada::exception(webstrada::string("Application"),
            webstrada::string(("The directory (" + dir + ") specified in the directory attribute in the cfcache tag does not exist.").c_str()),
            webstrada::string(""));
    }
}

// CF's setProtocol validation: only http/https (case-insensitive).
void validateProtocol(const cfvariant *protocol)
{
    if (!protocol) return;
    std::string p = trimmedLower(variantStr(protocol));
    if (p != "http" && p != "https") {
        throw webstrada::exception(webstrada::string("Application"),
            webstrada::string((p + " specified for attribute protocol is invalid.").c_str()),
            webstrada::string("Valid values are HTTP:// or HTTPS://."));
    }
}

// Validate an object-cache id (CacheTagHelper.validateID): must be non-empty
// after trim; null/empty -> CF's InvalidAttributeException.
std::string validateId(const cfvariant *id)
{
    std::string s = variantStr(id);
    size_t b = 0, e = s.size();
    while (b < e && (unsigned char)s[b] <= 0x20) b++;
    while (e > b && (unsigned char)s[e-1] <= 0x20) e--;
    std::string trimmed = s.substr(b, e - b);
    if (trimmed.empty()) {
        throw webstrada::exception(webstrada::string("Application"),
            webstrada::string("Attribute validation error for the CFCACHE tag."),
            webstrada::string(""));
    }
    return trimmed;
}

cfvariant *serializeValue(const cfvariant *value)
{
    return cf_serializejson(value, nullptr, nullptr, nullptr);
}

cfvariant *deserializeValue(const std::string &json)
{
    cfvariant jsonVal(json.c_str());
    return cf_deserializejson(&jsonVal, nullptr, true);
}

// Build the metadata struct (same keys as cacheGetMetadata).
cfvariant *makeMetadataStruct(const webstrada::CacheStore::EntryMeta &meta)
{
    auto *ret = new cfvariant(cfvariant::Struct);
    if (!meta.found) return ret;
    ret->set("NAME") = cfvariant(meta.name.c_str());
    {
        cfvariant v(cfvariant::Long);
        v.m_long = meta.timetolive;
        ret->set("TIMESPAN") = v;
    }
    {
        cfvariant v(cfvariant::Long);
        v.m_long = meta.timetoidle;
        ret->set("IDLETIME") = v;
    }
    {
        cfvariant v(cfvariant::Long);
        v.m_long = meta.hits;
        ret->set("HITCOUNT") = v;
    }
    {
        time_t unixTime = static_cast<time_t>(meta.createdMs / 1000);
        struct tm tmv;
        memset(&tmv, 0, sizeof(tmv));
        gmtime_r(&unixTime, &tmv);
        cfvariant dt(cfvariant::DateTime);
        dt.m_double = tmToDays(tmv);
        ret->set("CREATEDTIME") = dt;
    }
    {
        time_t unixTime = static_cast<time_t>(meta.lastAccessMs / 1000);
        struct tm tmv;
        memset(&tmv, 0, sizeof(tmv));
        gmtime_r(&unixTime, &tmv);
        cfvariant dt(cfvariant::DateTime);
        dt.m_double = tmToDays(tmv);
        ret->set("LASTHIT") = dt;
    }
    {
        time_t unixTime = static_cast<time_t>(meta.lastUpdateMs / 1000);
        struct tm tmv;
        memset(&tmv, 0, sizeof(tmv));
        gmtime_r(&unixTime, &tmv);
        cfvariant dt(cfvariant::DateTime);
        dt.m_double = tmToDays(tmv);
        ret->set("LASTUPDATED") = dt;
    }
    {
        cfvariant v(cfvariant::Long);
        v.m_long = meta.size;
        ret->set("SIZE") = v;
    }
    return ret;
}

void setVariable(void *variables, const std::string &name, cfvariant *value)
{
    if (!variables || name.empty()) {
        delete value;
        return;
    }
    cfvariant *v = static_cast<cfvariant*>(variables);
    if (v->m_type != cfvariant::Struct || !v->m_struct) {
        delete value;
        return;
    }
    v->set(name.c_str()) = *value;
    delete value;
}

// Remove a page-scope variable. CF's <cfcache action="get"> on a cache miss
// calls pageContext.setAttribute(name, null), which removes the attribute, so
// the name becomes undefined (isDefined -> NO, even if it had a value before).
void removeVariable(void *variables, const std::string &name)
{
    if (!variables || name.empty()) return;
    cfvariant *v = static_cast<cfvariant*>(variables);
    if (v->m_type != cfvariant::Struct || !v->m_struct) return;
    struct_data_bump(v->m_structData);
    v->m_struct->erase(name.c_str());
}

// ---- actions -------------------------------------------------------------

// CacheTag.addHTTPHeaders: Expires / Cache-Control / Pragma client headers.
void addClientCacheHeaders(int64_t timespanSec)
{
    auto &r = response();
    int64_t nowSec = static_cast<int64_t>(::time(nullptr));
    responseSetHeader(r, "Expires", webstrada::string(httpDateString(nowSec + timespanSec).c_str()));
    responseSetHeader(r, "Cache-Control", webstrada::string(("public,max-age=" + std::to_string(timespanSec)).c_str()));
    responseSetHeader(r, "Pragma", "public");
}

// Object-cache get (action="get").
void doCacheGet(void *variables, const cfvariant *id, const cfvariant *key,
                const cfvariant *region, const cfvariant *name,
                const cfvariant *metadata)
{
    std::string idS = validateId(id);
    std::string regionName = resolveRegion(region, key, "OBJECT");
    cfvariant *cached = nullptr;
    if (cacheOpen()) {
        std::string valueJson;
        if (store().get(regionName, idS, nowMs(), valueJson, /*quiet=*/false)) {
            cached = deserializeValue(valueJson);
        }
    }
    if (name) {
        if (cached) {
            setVariable(variables, variantStr(name), cached);
        } else {
            // CF sets the variable to null on a miss, which removes it (the
            // name becomes undefined).
            removeVariable(variables, variantStr(name));
        }
    } else if (cached) {
        delete cached;
    }
    if (metadata) {
        webstrada::CacheStore::EntryMeta meta;
        if (cacheOpen()) meta = store().metadata(regionName, idS, nowMs());
        setVariable(variables, variantStr(metadata), makeMetadataStruct(meta));
    }
}

// Object-cache put (action="put").
void doCachePut(const cfvariant *id, const cfvariant *value, const cfvariant *key,
                const cfvariant *region, int64_t timespanSec, int64_t idleSec)
{
    std::string idS = validateId(id);
    if (!value) {
        throw webstrada::exception(webstrada::string("Template"),
            webstrada::string("Attribute validation error for cfcache."),
            webstrada::string(""));
    }
    std::string regionName = resolveRegion(region, key, "OBJECT");
    if (!cacheOpen()) return;
    cfvariant *json = serializeValue(value);
    webstrada::string jstr = json->toString();
    std::string valueJson = jstr.constData() ? jstr.constData() : "";
    delete json;
    store().put(regionName, idS, valueJson,
                resolveIntervalMs(timespanSec), resolveIntervalMs(idleSec), nowMs());
}

// Flush the template cache: entries matching the expireURL pattern (or all
// when no pattern) are removed from the template region.
void flushTemplateCache(const cfvariant *expireurl, const cfvariant *key,
                        const cfvariant *region)
{
    if (!cacheOpen()) return;
    std::string regionName = resolveRegion(region, key, "TEMPLATE");
    std::string pattern = expireurl ? trimmedLower(variantStr(expireurl)) : std::string();
    int64_t now = nowMs();
    std::vector<std::string> ids = store().getAllIds(regionName, now, /*includeExpired=*/true);
    if (pattern.empty()) {
        for (const auto &id : ids) {
            store().remove(regionName, id);
        }
        return;
    }
    // CacheTagHelper.createMatchCriteria: + and ? -> _query_, * -> .*
    std::string regex;
    for (char c : pattern) {
        if (c == '+' || c == '?') regex += "_query_";
        else if (c == '*') regex += ".*";
        else regex += c;
    }
    try {
        std::regex re(regex, std::regex::icase);
        for (const auto &id : ids) {
            if (std::regex_match(id, re)) {
                store().remove(regionName, id);
            }
        }
    } catch (const std::regex_error &) {
        // A malformed pattern removes nothing (like CF's non-matching flush).
    }
}

// Flush the object cache by id (comma-separated), honoring throwOnError.
void flushObjectCache(const cfvariant *id, bool throwOnError, const cfvariant *key,
                      const cfvariant *region)
{
    if (!cacheOpen()) return;
    std::string regionName = resolveRegion(region, key, "OBJECT");
    std::string idStr = variantStr(id);
    if (idStr.empty()) {
        if (throwOnError) {
            throw webstrada::exception(webstrada::string("Application"),
                webstrada::string("Attribute validation error for the CFCACHE tag."),
                webstrada::string(""));
        }
        return;
    }
    std::vector<std::string> failed;
    size_t start = 0;
    while (start <= idStr.size()) {
        size_t comma = idStr.find(',', start);
        std::string item = idStr.substr(start, comma == std::string::npos ? std::string::npos : comma - start);
        size_t b = 0, e = item.size();
        while (b < e && (unsigned char)item[b] <= 0x20) b++;
        while (e > b && (unsigned char)item[e-1] <= 0x20) e--;
        std::string trimmed = item.substr(b, e - b);
        if (!trimmed.empty()) {
            if (!store().remove(regionName, trimmed)) {
                failed.push_back(trimmed);
            }
        }
        if (comma == std::string::npos) break;
        start = comma + 1;
    }
    if (throwOnError && !failed.empty()) {
        std::string list;
        for (size_t i = 0; i < failed.size(); i++) {
            if (i) list += ",";
            list += failed[i];
        }
        throw webstrada::exception(webstrada::string("Application"),
            webstrada::string("Could not remove some ids from object cache."),
            webstrada::string(("IDs not found in cache:" + list).c_str()));
    }
}

cfvariant *makeBool(bool b)
{
    auto *ret = new cfvariant(cfvariant::Boolean);
    ret->m_bool = b;
    return ret;
}

// Push a pending fragment store for the body form (only on the miss path).
void pushFragmentCapture(webstrada::string *out, bool useURL,
                         void *cgi, bool hasEndTag, int lineNo,
                         const std::vector<std::string> &dependsOn,
                         const cfvariant *key, const cfvariant *region,
                         bool stripWs, int64_t timespanSec, int64_t idleSec,
                         const cfvariant *metadata)
{
    FragCapture cap;
    cap.startLen = out->length();
    cap.region = resolveRegion(region, key, "TEMPLATE");
    cap.key = templateCacheKey(useURL, cgi, hasEndTag, lineNo, dependsOn);
    cap.stripWhitespace = stripWs;
    cap.ttlMs = resolveIntervalMs(timespanSec);
    cap.idleMs = resolveIntervalMs(idleSec);
    cap.metadataName = metadata ? variantStr(metadata) : std::string();
    g_fragCaptures.push_back(cap);
}

} // namespace

// ---- <cfcache> start tag -------------------------------------------------

cfvariant *cf_cache_tag_begin(
    webstrada::string *out, void *cgi, void *server, void *cookie, void *application,
    void *session, void *url, void *form, void *variables,
    const cfvariant *action, const cfvariant *directory,
    const cfvariant *timespan, const cfvariant *idletime,
    const cfvariant *expireurl,
    const cfvariant *username, const cfvariant *password,
    const cfvariant *protocol, const cfvariant *port,
    const cfvariant *id, const cfvariant *key, const cfvariant *region,
    const cfvariant *dependson, const cfvariant *usecache,
    const cfvariant *stripwhitespace, const cfvariant *value,
    const cfvariant *name, const cfvariant *metadata,
    const cfvariant *throwonerror, const cfvariant *usequerystring,
    int hasEndTag, int lineNo)
{
    (void)server; (void)cookie; (void)application; (void)session; (void)url;
    (void)form; (void)username; (void)password; (void)port;

    // The action string (CF's default is SERVERCACHE).
    std::string act = action ? trimmedLower(variantStr(action)) : "servercache";

    // Runtime validations that CF's doStartTag performs before dispatch.
    if (region && key) {
        throw webstrada::exception(webstrada::string("Application"),
            webstrada::string("Key and Region attributes cannot be specified together."),
            webstrada::string("Key attribute is deprecated."));
    }
    if (metadata && !hasEndTag &&
        (act == "cache" || act == "servercache" || act == "optimal")) {
        throw webstrada::exception(webstrada::string("Application"),
            webstrada::string("Attribute metadata cannot be used without an end tag."),
            webstrada::string(""));
    }
    if (!hasEndTag && (act == "cache" || act == "servercache" || act == "optimal")) {
        cfml::IncludeRuntime *rt = cfml::include_context();
        if (rt && rt->includeDepth > 0) {
            throw webstrada::exception(webstrada::string("Application"),
                webstrada::string("CFCache must have an end tag if used inside an included template."),
                webstrada::string(""));
        }
    }
    validateProtocol(protocol);
    validateDirectory(directory);

    // FLUSH with both id and expireurl is a runtime error (CF's
    // ExpireUrlIdDefinedException; the static case is rejected at compile time
    // by the TLD combination validation).
    if (act == "flush" && id && expireurl) {
        throw webstrada::exception(webstrada::string("Application"),
            webstrada::string("Attribute validation error for the CFCACHE tag."),
            webstrada::string("ID and EXPIREURL cannot be specified together for action FLUSH."));
    }

    // action="get"/"put" require attributes at runtime too (dynamic action).
    if (act == "get" && (!name || !id)) {
        throw webstrada::exception(webstrada::string("Template"),
            webstrada::string("Attribute validation error for cfcache."),
            webstrada::string(""));
    }
    if (act == "put" && (!id || !value)) {
        throw webstrada::exception(webstrada::string("Template"),
            webstrada::string("Attribute validation error for cfcache."),
            webstrada::string(""));
    }

    bool useCache = boolAttr(usecache, true);
    bool stripWs = boolAttr(stripwhitespace, false);
    bool useURL = boolAttr(usequerystring, false);
    int64_t timespanSec = secondsFromDays(timespan);
    int64_t idleSec = secondsFromDays(idletime);
    std::vector<std::string> dependsOn = parseDependsOn(dependson, variables);

    if (act == "get") {
        doCacheGet(variables, id, key, region, name, metadata);
        return makeBool(hasEndTag ? true : false);
    }
    if (act == "put") {
        doCachePut(id, value, key, region, timespanSec, idleSec);
        return makeBool(hasEndTag ? true : false);
    }
    if (act == "flush") {
        // With useCache, flush sets foundCache=true (no whole-page store);
        // with usecache=false the tag behaves like a render and the whole page
        // is still stored (CF's doEndTag sets cacheConfig when !foundCache).
        if (useCache) {
            if (id) {
                flushObjectCache(id, boolAttr(throwonerror, false), key, region);
            } else {
                flushTemplateCache(expireurl, key, region);
            }
            return makeBool(hasEndTag ? true : false);
        }
        if (!hasEndTag) {
            g_pageCacheCfg.active = true;
            g_pageCacheCfg.key = templateCacheKey(useURL, cgi, hasEndTag, lineNo, dependsOn);
            g_pageCacheCfg.region = resolveRegion(region, key, "TEMPLATE");
            g_pageCacheCfg.ttlMs = resolveIntervalMs(timespanSec);
            g_pageCacheCfg.idleMs = resolveIntervalMs(idleSec);
            g_pageCacheCfg.stripWhitespace = stripWs;
        }
        return makeBool(false);
    }

    // cache / servercache / optimal / clientcache.
    std::string regionName = resolveRegion(region, key, "TEMPLATE");
    std::string cacheKey = templateCacheKey(useURL, cgi, hasEndTag, lineNo, dependsOn);

    auto registerPageStore = [&]() {
        g_pageCacheCfg.active = true;
        g_pageCacheCfg.key = cacheKey;
        g_pageCacheCfg.region = regionName;
        g_pageCacheCfg.ttlMs = resolveIntervalMs(timespanSec);
        g_pageCacheCfg.idleMs = resolveIntervalMs(idleSec);
        g_pageCacheCfg.stripWhitespace = stripWs;
    };

    if (!useCache) {
        // Skip the cache check and the client headers; the body still renders
        // (a body-form fragment is still stored, like CF's doAfterBody; a
        // self-closing tag still registers the whole-page store because CF's
        // doEndTag sets cacheConfig when foundCache is false).
        if (hasEndTag) {
            pushFragmentCapture(out, useURL, cgi, hasEndTag, lineNo,
                                dependsOn, key, region, stripWs, timespanSec, idleSec, metadata);
        } else {
            registerPageStore();
        }
        return makeBool(false);
    }

    if (act == "clientcache") {
        addClientCacheHeaders(timespanSec);
        if (hasEndTag) {
            pushFragmentCapture(out, useURL, cgi, hasEndTag, lineNo,
                                dependsOn, key, region, stripWs, timespanSec, idleSec, metadata);
        } else {
            // clientcache never sets foundCache, so the whole page is stored.
            registerPageStore();
        }
        return makeBool(false);
    }

    // Template cache actions (cache/servercache/optimal).
    if (!cacheOpen()) {
        // No cache store: behave like a miss (render normally; nothing stored).
        if (hasEndTag) {
            pushFragmentCapture(out, useURL, cgi, hasEndTag, lineNo,
                                dependsOn, key, region, stripWs, timespanSec, idleSec, metadata);
        }
        return makeBool(false);
    }

    if (hasEndTag) {
        // Fragment form: on a hit write the cached fragment and skip the body.
        std::string valueJson;
        if (store().get(regionName, cacheKey, nowMs(), valueJson, /*quiet=*/false)) {
            std::string body;
            if (deserializeFragment(valueJson, body)) {
                out->append(body.c_str(), body.size());
                if (metadata) {
                    webstrada::CacheStore::EntryMeta meta = store().metadata(regionName, cacheKey, nowMs());
                    setVariable(variables, variantStr(metadata), makeMetadataStruct(meta));
                }
                return makeBool(true);
            }
        }
        // Miss: capture the body output for the end tag.
        pushFragmentCapture(out, useURL, cgi, hasEndTag, lineNo,
                            dependsOn, key, region, stripWs, timespanSec, idleSec, metadata);
        return makeBool(false);
    }

    // Whole-page (self-closing) form.
    std::string valueJson;
    if (store().get(regionName, cacheKey, nowMs(), valueJson, /*quiet=*/false)) {
        response_state &r = response();
        std::string body;
        if (deserializePage(valueJson, r, body)) {
            // CF's writeResponse + getOut().clear(): drop any output written
            // before the <cfcache> tag and serve the cached page.
            out->clear();
            out->append(body.c_str(), body.size());
            return makeBool(true);
        }
    }
    // Miss: register the whole-page store; the harness stores it after the
    // page completes. For cache/optimal the first (miss) request also gets the
    // client-cache headers, captured into the stored page.
    if (act == "cache" || act == "optimal") {
        addClientCacheHeaders(timespanSec);
    }
    registerPageStore();
    return makeBool(false);
}

// ---- <cfcache> end tag ---------------------------------------------------

void cf_cache_tag_end(webstrada::string *out, void *variables)
{
    if (g_fragCaptures.empty()) return;
    FragCapture cap = g_fragCaptures.back();
    g_fragCaptures.pop_back();
    if (!cacheOpen()) return;
    if (out->length() <= (int)cap.startLen) return;
    webstrada::string body = out->mid(cap.startLen, out->length() - cap.startLen);
    std::string bodyStr = body.constData() ? std::string(body.constData(), body.length()) : std::string();
    if (cap.stripWhitespace) {
        bodyStr = stripWhitespaces(bodyStr);
        // CF's addToTemplateCache re-prints the stripped body to the page, so
        // the displayed fragment on the miss path is stripped too.
        out->remove(cap.startLen, out->length() - cap.startLen);
        out->append(bodyStr.c_str(), bodyStr.size());
    }
    store().put(cap.region, cap.key, serializeFragment(bodyStr),
                cap.ttlMs, cap.idleMs, nowMs());
    // The `metadata` variable is set on the miss path after the fragment is
    // stored (CF's doAfterBody), like the hit path in cf_cache_tag_begin.
    if (!cap.metadataName.empty()) {
        webstrada::CacheStore::EntryMeta meta = store().metadata(cap.region, cap.key, nowMs());
        setVariable(variables, cap.metadataName, makeMetadataStruct(meta));
    }
}

// ---- whole-page store hook ----------------------------------------------

void cf_cache_reset()
{
    g_pageCacheCfg = PageCacheCfg();
    g_fragCaptures.clear();
}

void cf_cache_store_page(webstrada::string *out)
{
    if (!g_pageCacheCfg.active) return;
    g_pageCacheCfg.active = false;
    if (!cacheOpen()) return;
    response_state &r = response();
    std::string body = out->constData() ? std::string(out->constData(), out->length()) : std::string();
    if (g_pageCacheCfg.stripWhitespace && r.contentType.contains("text/html")) {
        body = stripWhitespaces(body);
    }
    std::string blob = serializePage(r, body);
    store().put(g_pageCacheCfg.region, g_pageCacheCfg.key, blob,
                g_pageCacheCfg.ttlMs, g_pageCacheCfg.idleMs, nowMs());
}

} // namespace cfml
