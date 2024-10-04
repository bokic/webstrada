/**
 * @file fn_cache.cpp
 * @brief CFML cacheGet()/cachePut()/cacheRegion*()/removeCachedQuery() built-ins.
 */

#include "common.h"

#include "../cftags/common.h"
#include <webstrada/cf8.h>
#include <webstrada/cache_store.h>
#include <webstrada/cfvariant.h>
#include <webstrada/config.h>
#include <webstrada/exceptions.h>
#include <webstrada/string.h>

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace cfml {

namespace {

// Cache values are stored as SerializeJSON blobs (the engine's object wire
// format, matching ObjectSave). `serialize` returns a JSON string for a value;
// `deserialize` reverses it, returning a heap cfvariant.
cfvariant *serializeCacheValue(const cfvariant *value)
{
    return cf_serializejson(value, nullptr, nullptr, nullptr);
}

cfvariant *deserializeCacheValue(const std::string &json)
{
    cfvariant jsonVal(json.c_str());
    return cf_deserializejson(&jsonVal, nullptr, true);
}

bool boolArg(const cfvariant *v, bool def)
{
    return v ? cf_is_truthy_value(v) : def;
}

// Resolve the region name: an explicit region arg (trimmed, uppercased for
// CF's standard regions OBJECT/TEMPLATE/QUERY) or the default "OBJECT".
std::string resolveRegion(const cfvariant *region)
{
    if (!region) return "OBJECT";
    webstrada::string s = const_cast<cfvariant*>(region)->toString();
    std::string r = s.constData() ? s.constData() : "";
    // Trim.
    size_t b = 0, e = r.size();
    while (b < e && (unsigned char)r[b] <= 0x20) b++;
    while (e > b && (unsigned char)r[e-1] <= 0x20) e--;
    r = r.substr(b, e - b);
    if (r.empty()) return "OBJECT";
    if (r.size() >= 5) {
        std::string up;
        for (char c : r) up += (char)toupper((unsigned char)c);
        if (up == "OBJECT" || up == "QUERY" || up == "TEMPLATE") return up;
    }
    return r;
}

// Parse a timeSpan / idleTime argument: a decimal number of days (from
// CreateTimeSpan), converted to milliseconds. Returns -1 for null or an empty
// string, meaning "not specified" (the caller applies the region's default
// TTL/idle, mirroring CF's addToObjectCache which maps "" to -1 and
// GenericEhcache.put which maps a negative interval to the cache config
// default). A literal 0 stays 0; when BOTH intervals are 0 the entry is
// eternal, exactly like CF.
int64_t parseIntervalMs(const cfvariant *v)
{
    if (!v) return -1;
    webstrada::string s = const_cast<cfvariant*>(v)->toString();
    std::string str = s.constData() ? s.constData() : "";
    size_t b = 0, e = str.size();
    while (b < e && (unsigned char)str[b] <= 0x20) b++;
    while (e > b && (unsigned char)str[e-1] <= 0x20) e--;
    std::string trimmed = str.substr(b, e - b);
    if (trimmed.empty()) return -1;
    double days = getDoubleValue(*v);
    if (days < 0) return -1;
    return (int64_t)(days * 86400.0 * 1000.0);
}

// Resolve a raw interval into the milliseconds actually stored: a negative
// "not specified" value uses CF's default cache config (1 day), 0+0 stays
// eternal.
int64_t resolveIntervalMs(int64_t raw)
{
    return raw < 0 ? 86400LL * 1000LL : raw;
}

// Convert a unix-epoch milliseconds value into a CFML DateTime (days since
// 1899-12-30, UTC), matching how GetHttpTimeString builds a date.
cfvariant *makeDateTimeMs(int64_t ms)
{
    time_t unixTime = static_cast<time_t>(ms / 1000);
    struct tm tmv;
    memset(&tmv, 0, sizeof(tmv));
    gmtime_r(&unixTime, &tmv);
    cfvariant *ret = new cfvariant(cfvariant::DateTime);
    ret->m_double = tmToDays(tmv);
    cf_register_temp(ret);
    return ret;
}

void throwCacheNotOpen()
{
    throw webstrada::exception(webstrada::string("Application"),
                              webstrada::string("The cache store is not available in this context."),
                              webstrada::string(""));
}

webstrada::string concat(const std::string &a, const std::string &b)
{
    return webstrada::string((a + b).c_str());
}

} // namespace

cfvariant *cf_cacheget(const cfvariant *id, const cfvariant *region)
{
    if (!id) throw webstrada::exception("The id parameter to the cacheGet function is required.");
    webstrada::string idStr = const_cast<cfvariant*>(id)->toString();
    std::string idS = idStr.constData() ? idStr.constData() : "";
    if (idS.empty()) throw webstrada::exception("The id parameter to the cacheGet function is required.");

    webstrada::CacheStore &store = webstrada::cache_store();
    if (!store.isOpen()) throwCacheNotOpen();

    std::string regionName = resolveRegion(region);
    std::string valueJson;
    int64_t now = static_cast<int64_t>(::time(nullptr)) * 1000;
    if (!store.get(regionName, idS, now, valueJson, /*quiet=*/false)) {
        return nullResult();
    }
    return deserializeCacheValue(valueJson);
}

cfvariant *cf_cacheput(const cfvariant *id, const cfvariant *value, const cfvariant *timespan,
                       const cfvariant *idleTime, const cfvariant *region,
                       const cfvariant *throwOnError)
{
    if (!id) throw webstrada::exception("The id parameter to the cachePut function is required.");
    if (!value) throw webstrada::exception("The value parameter to the cachePut function is required.");
    webstrada::string idStr = const_cast<cfvariant*>(id)->toString();
    std::string idS = idStr.constData() ? idStr.constData() : "";
    if (idS.empty()) throw webstrada::exception("The id parameter to the cachePut function is required.");

    webstrada::CacheStore &store = webstrada::cache_store();
    if (!store.isOpen()) throwCacheNotOpen();

    std::string regionName = resolveRegion(region);
    if (boolArg(throwOnError, false) && !store.regionExists(regionName) &&
        regionName != "OBJECT" && regionName != "TEMPLATE" && regionName != "QUERY") {
        throw webstrada::exception(webstrada::string("Application"), concat("Cache ", regionName) + " was not found.", webstrada::string(""));
    }

    cfvariant *json = serializeCacheValue(value);
    webstrada::string jstr = json->toString();
    std::string valueJson = jstr.constData() ? jstr.constData() : "";
    delete json;

    int64_t ttl = resolveIntervalMs(parseIntervalMs(timespan));
    int64_t idle = resolveIntervalMs(parseIntervalMs(idleTime));
    int64_t now = static_cast<int64_t>(::time(nullptr)) * 1000;
    store.put(regionName, idS, valueJson, ttl, idle, now);

    return cfvariant_create_null();
}

cfvariant *cf_cacheidexists(const cfvariant *id, const cfvariant *region)
{
    if (!id) throw webstrada::exception("The id parameter to the cacheIdExists function is required.");
    webstrada::string idStr = const_cast<cfvariant*>(id)->toString();
    std::string idS = idStr.constData() ? idStr.constData() : "";

    webstrada::CacheStore &store = webstrada::cache_store();
    if (!store.isOpen()) throwCacheNotOpen();

    std::string regionName = resolveRegion(region);
    if (!store.regionExists(regionName)) {
        auto *ret = new cfvariant(cfvariant::Boolean);
        ret->m_bool = false;
        return ret;
    }
    int64_t now = static_cast<int64_t>(::time(nullptr)) * 1000;
    auto *ret = new cfvariant(cfvariant::Boolean);
    ret->m_bool = store.idExists(regionName, idS, now);
    return ret;
}

cfvariant *cf_cachegetallids(const cfvariant *region, const cfvariant *includeExpired)
{
    webstrada::CacheStore &store = webstrada::cache_store();
    if (!store.isOpen()) throwCacheNotOpen();

    std::string regionName = resolveRegion(region);
    int64_t now = static_cast<int64_t>(::time(nullptr)) * 1000;
    bool inclExpired = boolArg(includeExpired, false);
    std::vector<std::string> ids = store.getAllIds(regionName, now, inclExpired);

    auto *ret = new cfvariant(cfvariant::Array);
    for (const auto &id : ids) {
        cfvariant v(id.c_str());
        ret->insert(v);
    }
    return ret;
}

cfvariant *cf_cachegetmetadata(const cfvariant *id, const cfvariant *objectType, const cfvariant *region)
{
    if (!id) throw webstrada::exception("The id parameter to the cacheGetMetadata function is required.");
    webstrada::string idStr = const_cast<cfvariant*>(id)->toString();
    std::string idS = idStr.constData() ? idStr.constData() : "";

    webstrada::CacheStore &store = webstrada::cache_store();
    if (!store.isOpen()) throwCacheNotOpen();

    std::string regionName = resolveRegion(region);
    int64_t now = static_cast<int64_t>(::time(nullptr)) * 1000;
    webstrada::CacheStore::EntryMeta meta = store.metadata(regionName, idS, now);

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
    ret->set("CREATEDTIME") = *makeDateTimeMs(meta.createdMs);
    ret->set("LASTHIT") = *makeDateTimeMs(meta.lastAccessMs);
    ret->set("LASTUPDATED") = *makeDateTimeMs(meta.lastUpdateMs);
    {
        cfvariant v(cfvariant::Long);
        v.m_long = meta.size;
        ret->set("SIZE") = v;
    }
    return ret;
}

cfvariant *cf_cachegetproperties(const cfvariant *region)
{
    webstrada::CacheStore &store = webstrada::cache_store();
    if (!store.isOpen()) throwCacheNotOpen();

    std::string regionName = resolveRegion(region);
    // CF's CacheGetProperties returns the named region's properties.
    auto *ret = new cfvariant(cfvariant::Struct);
    std::string props = store.regionProperties(regionName);
    if (!props.empty() && props != "{}") {
        cfvariant jsonVal(props.c_str());
        cfvariant *parsed = cf_deserializejson(&jsonVal, nullptr, true);
        if (parsed && parsed->m_type == cfvariant::Struct) {
            for (auto &kv : *parsed->m_struct) {
                ret->set(kv.first) = kv.second;
            }
            delete parsed;
        } else if (parsed) {
            delete parsed;
        }
    }
    ret->set("NAME") = cfvariant(regionName.c_str());
    return ret;
}

cfvariant *cf_cachesetproperties(const cfvariant *properties, const cfvariant *region)
{
    if (!properties || properties->m_type != cfvariant::Struct || !properties->m_struct) {
        throw webstrada::exception("The properties parameter to the cacheSetProperties function must be a struct.");
    }
    webstrada::CacheStore &store = webstrada::cache_store();
    if (!store.isOpen()) throwCacheNotOpen();

    std::string regionName = resolveRegion(region);
    // Merge the new properties over the existing ones.
    std::string existing = store.regionProperties(regionName);
    cfvariant base(cfvariant::Struct);
    if (!existing.empty() && existing != "{}") {
        cfvariant jsonVal(existing.c_str());
        cfvariant *parsed = cf_deserializejson(&jsonVal, nullptr, true);
        if (parsed && parsed->m_type == cfvariant::Struct) {
            for (auto &kv : *parsed->m_struct) {
                base.set(kv.first) = kv.second;
            }
        }
        if (parsed) delete parsed;
    }
    for (auto &kv : *properties->m_struct) {
        base.set(kv.first) = kv.second;
    }
    cfvariant *json = serializeCacheValue(&base);
    webstrada::string jstr = json->toString();
    std::string merged = jstr.constData() ? jstr.constData() : "";
    delete json;
    store.setRegionProperties(regionName, merged);
    return cfvariant_create_null();
}

cfvariant *cf_cacheregionexists(const cfvariant *region)
{
    if (!region) throw webstrada::exception("The region parameter to the cacheRegionExists function is required.");
    webstrada::CacheStore &store = webstrada::cache_store();
    if (!store.isOpen()) throwCacheNotOpen();
    std::string regionName = resolveRegion(region);
    auto *ret = new cfvariant(cfvariant::Boolean);
    ret->m_bool = store.regionExists(regionName);
    return ret;
}

cfvariant *cf_cacheregionnew(const cfvariant *region, const cfvariant *properties, const cfvariant *throwOnError)
{
    if (!region) throw webstrada::exception("The region parameter to the cacheRegionNew function is required.");
    webstrada::string regionStr = const_cast<cfvariant*>(region)->toString();
    std::string regionName = resolveRegion(region);
    if (regionName.empty()) throw webstrada::exception("The region parameter to the cacheRegionNew function is required.");

    webstrada::CacheStore &store = webstrada::cache_store();
    if (!store.isOpen()) throwCacheNotOpen();

    if (store.regionExists(regionName)) {
        if (boolArg(throwOnError, true)) {
            throw webstrada::exception(webstrada::string("Application"), concat("Cache ", regionName) + " already exists.", webstrada::string(""));
        }
        return cfvariant_create_null();
    }
    store.createRegion(regionName);
    if (properties && properties->m_type == cfvariant::Struct) {
        cfvariant *json = serializeCacheValue(properties);
        webstrada::string jstr = json->toString();
        std::string props = jstr.constData() ? jstr.constData() : "";
        delete json;
        store.setRegionProperties(regionName, props);
    }
    return cfvariant_create_null();
}

cfvariant *cf_cacheregionremove(const cfvariant *region)
{
    if (!region) throw webstrada::exception("The region parameter to the cacheRegionRemove function is required.");
    std::string regionName = resolveRegion(region);
    if (regionName == "OBJECT" || regionName == "TEMPLATE" || regionName == "QUERY") {
        throw webstrada::exception(webstrada::string("Application"), concat("Cache ", regionName) + " cannot be removed.", webstrada::string(""));
    }
    webstrada::CacheStore &store = webstrada::cache_store();
    if (!store.isOpen()) throwCacheNotOpen();
    if (!store.regionExists(regionName)) {
        throw webstrada::exception(webstrada::string("Application"), concat("Cache ", regionName) + " was not found.", webstrada::string(""));
    }
    store.removeRegion(regionName);
    return cfvariant_create_null();
}

cfvariant *cf_cacheremove(const cfvariant *id, const cfvariant *throwOnError,
                          const cfvariant *region, const cfvariant *exact)
{
    webstrada::CacheStore &store = webstrada::cache_store();
    if (!store.isOpen()) throwCacheNotOpen();
    std::string regionName = resolveRegion(region);

    // id may be a comma-delimited list or an array of ids.
    std::vector<std::string> ids;
    if (id) {
        if (id->m_type == cfvariant::Array && id->m_array) {
            for (auto &v : *id->m_array) {
                webstrada::string s = v.toString();
                ids.push_back(s.constData() ? s.constData() : "");
            }
        } else {
            webstrada::string s = const_cast<cfvariant*>(id)->toString();
            std::string str = s.constData() ? s.constData() : "";
            size_t start = 0;
            while (start <= str.size()) {
                size_t comma = str.find(',', start);
                std::string item = str.substr(start, comma == std::string::npos ? std::string::npos : comma - start);
                size_t b = 0, e = item.size();
                while (b < e && (unsigned char)item[b] <= 0x20) b++;
                while (e > b && (unsigned char)item[e-1] <= 0x20) e--;
                ids.push_back(item.substr(b, e - b));
                if (comma == std::string::npos) break;
                start = comma + 1;
            }
        }
    }
    bool exactMatch = boolArg(exact, true);
    bool throwErr = boolArg(throwOnError, false);

    int64_t now = static_cast<int64_t>(::time(nullptr)) * 1000;
    std::vector<std::string> removed;
    if (exactMatch) {
        for (const auto &i : ids) {
            if (!i.empty() && store.remove(regionName, i)) removed.push_back(i);
        }
    } else {
        // Non-exact: match ids with a leading/trailing * wildcard.
        for (const auto &i : ids) {
            if (i.empty()) continue;
            bool prefixStar = i.front() == '*';
            bool suffixStar = i.back() == '*';
            std::string core = i;
            if (prefixStar) core = core.substr(1);
            if (suffixStar && !core.empty()) core = core.substr(0, core.size() - 1);
            std::vector<std::string> all = store.getAllIds(regionName, now, true);
            for (const auto &cand : all) {
                bool match = true;
                if (prefixStar && suffixStar) match = cand.find(core) != std::string::npos;
                else if (prefixStar) match = cand.size() >= core.size() && cand.compare(cand.size() - core.size(), core.size(), core) == 0;
                else if (suffixStar) match = cand.rfind(core, 0) == 0;
                else match = cand == core;
                if (match && store.remove(regionName, cand)) removed.push_back(cand);
            }
        }
    }
    if (throwErr && ids.size() != removed.size()) {
        throw webstrada::exception(webstrada::string("Application"), webstrada::string("Cache remove failed for some ids."), webstrada::string(""));
    }
    return cfvariant_create_null();
}

cfvariant *cf_cacheremoveall(const cfvariant *region)
{
    webstrada::CacheStore &store = webstrada::cache_store();
    if (!store.isOpen()) throwCacheNotOpen();
    std::string regionName = resolveRegion(region);
    store.removeAll(regionName);
    return cfvariant_create_null();
}

cfvariant *cf_cachegetsession(const cfvariant *objectType, const cfvariant *isKey)
{
    // CF returns the underlying Java cache object; this engine has no Java
    // object interop, so like GetPageContext this throws.
    throw webstrada::exception("Function CacheGetSession is not supported: it returns a Java cache object.");
}

// Compute the query-cache id, mirroring CF's QueryDetails.getHashcode2 /
// RemoveCachedQuery: `29*(29*dsnHash + sqlHash) + paramHash` (a Java
// String.hashCode of the lowercased dsn/sql), or the explicit cacheid.
static int javaHashLower(const std::string &s)
{
    unsigned int h = 0;
    for (char c : s) {
        char up = (char)toupper((unsigned char)c);
        h = h * 31u + (unsigned char)up;
    }
    return (int)h;
}

std::string cf_query_cache_key(const std::string &sql, const std::string &datasource,
                               const cfvariant *params, const cfvariant *cacheid)
{
    if (cacheid) {
        webstrada::string cid = const_cast<cfvariant*>(cacheid)->toString();
        return cid.constData() ? cid.constData() : "";
    }
    int result = datasource.empty() ? 0 : javaHashLower(datasource);
    int result2 = 29 * result + (sql.empty() ? 0 : javaHashLower(sql));
    int paramHash = 0;
    if (params && params->m_type == cfvariant::Array && params->m_array) {
        for (auto &v : *params->m_array) {
            if (v.m_type == cfvariant::Null) continue;
            webstrada::string s = v.toString();
            std::string str = s.constData() ? s.constData() : "";
            paramHash += (int)javaHashLower(str);
        }
    }
    int result3 = 29 * result2 + paramHash;
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%d", result3);
    return std::string(buf);
}


cfvariant *cf_removecachedquery(const cfvariant *sql, const cfvariant *datasource,
                                const cfvariant *params, const cfvariant *region)
{
    if (!sql) throw webstrada::exception("The sql parameter to the removeCachedQuery function is required.");
    webstrada::CacheStore &store = webstrada::cache_store();
    if (!store.isOpen()) throwCacheNotOpen();

    std::string regionName = "QUERY";
    if (region) {
        std::string r = resolveRegion(region);
        if (r == "OBJECT" || r == "TEMPLATE" || r == "QUERY") regionName = r;
        else regionName = r;
    }
    std::string sqlS = const_cast<cfvariant*>(sql)->toString().constData();
    std::string dsnS = datasource ? const_cast<cfvariant*>(datasource)->toString().constData() : "";
    std::string key = cf_query_cache_key(sqlS, dsnS, params, nullptr);
    store.remove(regionName, key);
    return cfvariant_create_null();
}

// Serialize a query into the cache blob: {"__WEBSTRADA_QUERY__":true,
// "COLUMNS":[{"name":..,"type":..,"VALUES":[...]}],"ROWCOUNT":N}.
cfvariant *cf_query_cache_serialize(const cfvariant *query)
{
    json_object *obj = json_object_new_object();
    json_object_object_add(obj, "__WEBSTRADA_QUERY__", json_object_new_boolean(1));
    if (query && query->m_type == cfvariant::Query && query->m_query) {
        const QueryData *qd = query->m_query;
        json_object *cols = json_object_new_array();
        for (auto &col : qd->columns) {
            json_object *c = json_object_new_object();
            json_object_object_add(c, "name", json_object_new_string(col.name.constData()));
            json_object_object_add(c, "type", json_object_new_string(col.type.constData()));
            json_object *vals = json_object_new_array();
            for (auto &v : col.values) {
                if (v.m_type == cfvariant::Null) {
                    json_object_array_add(vals, json_object_new_null());
                } else {
                    g_serializeVisited.clear();
                    json_object_array_add(vals, serialize_json_value(v, "", g_serializeVisited));
                }
            }
            json_object_object_add(c, "VALUES", vals);
            json_object_array_add(cols, c);
        }
        json_object_object_add(obj, "COLUMNS", cols);
        json_object_object_add(obj, "ROWCOUNT", json_object_new_int(qd->rowCount()));
    }
    const char *jsonStr = json_object_to_json_string_ext(obj, JSON_C_TO_STRING_PLAIN | JSON_C_TO_STRING_NOSLASHESCAPE);
    auto *ret = new cfvariant(jsonStr ? jsonStr : "");
    json_object_put(obj);
    return ret;
}

// Restore a query from the cache blob produced by cf_query_cache_serialize.
cfvariant *cf_query_cache_deserialize(const cfvariant *json)
{
    webstrada::string s = const_cast<cfvariant*>(json)->toString();
    std::string str = s.constData() ? s.constData() : "";
    if (str.empty()) return new cfvariant(cfvariant::Query);

    json_object *obj = json_tokener_parse(str.c_str());
    if (!obj) return new cfvariant(cfvariant::Query);
    auto *ret = new cfvariant(cfvariant::Query);
    QueryData *qd = ret->m_query;

    json_object *cols = nullptr;
    if (json_object_object_get_ex(obj, "COLUMNS", &cols)) {
        size_t n = json_object_array_length(cols);
        for (size_t i = 0; i < n; i++) {
            json_object *c = json_object_array_get_idx(cols, i);
            QueryColumn col;
            json_object *name = nullptr, *type = nullptr, *vals = nullptr;
            if (json_object_object_get_ex(c, "name", &name) && json_object_get_type(name) == json_type_string)
                col.name = json_object_get_string(name);
            if (json_object_object_get_ex(c, "type", &type) && json_object_get_type(type) == json_type_string)
                col.type = json_object_get_string(type);
            if (json_object_object_get_ex(c, "VALUES", &vals)) {
                size_t m = json_object_array_length(vals);
                for (size_t k = 0; k < m; k++) {
                    json_object *cell = json_object_array_get_idx(vals, k);
                    if (json_object_get_type(cell) == json_type_null) {
                        col.values.push_back(cfvariant(cfvariant::Null));
                    } else {
                        col.values.push_back(deserialize_json_value(cell, true, true));
                    }
                }
            }
            qd->columns.push_back(std::move(col));
        }
    }
    json_object *rc = nullptr;
    if (json_object_object_get_ex(obj, "ROWCOUNT", &rc)) {
        qd->m_rowCount = json_object_get_int(rc);
    } else {
        qd->m_rowCount = qd->columns.empty() ? 0 : (int)qd->columns[0].values.size();
    }
    json_object_put(obj);
    return ret;
}

} // namespace cfml
