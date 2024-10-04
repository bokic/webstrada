/**
 * @file tag_feed.cpp
 * @brief <cffeed> runtime (cf_feed_tag).
 *
 * Reads RSS/Atom feeds (action="read") and creates RSS 2.0 / Atom 1.0 feeds
 * (action="create") with Adobe ColdFusion semantics, ported from the
 * decompiled FeedTag / FeedReader / FeedGenerator / RSSParser / AtomParser /
 * RSSGenerator / AtomGenerator and the Rome WireFeedOutput pretty-print.
 *
 * The feed package is NOT installed on the CF 2025 RDS host (the bundle is in
 * the installer but not loaded — see BUGS_CF.md), so the byte-exact output and
 * error messages cannot be verified against a live server; the implementation
 * is pinned by unit tests with known RSS/Atom fixtures.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>
#include <webstrada/exceptions.h>
#include <webstrada/string.h>

#include <curl/curl.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <string>
#include <vector>

namespace {

using webstrada::cfvariant;
using webstrada::string;

// Read an attribute from the evaluated attribute struct (case-insensitive).
const cfvariant *attrOf(const cfvariant *attrs, const char *key)
{
    if (!attrs || attrs->m_type != cfvariant::Struct || !attrs->m_struct) return nullptr;
    string k(key);
    auto it = attrs->m_struct->find(k);
    return it == attrs->m_struct->end() ? nullptr : &it->second;
}

std::string attrStr(const cfvariant *attrs, const char *key)
{
    const cfvariant *v = attrOf(attrs, key);
    return v ? cfml::safe_to_std_string(*v) : std::string();
}

bool attrBool(const cfvariant *attrs, const char *key, bool def)
{
    const cfvariant *v = attrOf(attrs, key);
    return v ? cfml::cfmlBoolean(v, def) : def;
}

[[noreturn]] void throwApp(const std::string &message, const std::string &detail = "")
{
    throw webstrada::exception(webstrada::string("Application"),
        webstrada::string(message.c_str()), webstrada::string(detail.c_str()));
}

std::string lower(std::string s)
{
    for (auto &c : s) c = static_cast<char>(tolower((unsigned char)c));
    return s;
}

// The metadata struct value insert rule (only non-null, non-empty strings).
void insertNotNull(cfvariant &st, const char *key, const std::string &value)
{
    if (!value.empty()) {
        st.set(key) = cfvariant(value.c_str());
    }
}

// Reads a string attribute from a struct (case-insensitive).
std::string stGet(const cfvariant &st, const char *key)
{
    if (st.m_type != cfvariant::Struct || !st.m_struct) return "";
    string k(key);
    auto it = st.m_struct->find(k);
    if (it == st.m_struct->end()) return "";
    return cfml::safe_to_std_string(it->second);
}

// ---- date helpers ----

static const char *dayNames[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
static const char *monthNames[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                   "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

// Java SimpleDateFormat("EEE, dd MMM yyyy HH:mm:ss 'GMT'") in GMT.
std::string formatRFC822(const cfvariant &dt)
{
    struct tm tm = cfml::daysToTm(dt.m_double);
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%s, %02d %s %04d %02d:%02d:%02d GMT",
                  dayNames[tm.tm_wday], tm.tm_mday, monthNames[tm.tm_mon],
                  tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec);
    return buf;
}

// Java SimpleDateFormat("yyyy-MM-dd'T'HH:mm:ss'Z'") in GMT.
std::string formatW3CDateTime(const cfvariant &dt)
{
    struct tm tm = cfml::daysToTm(dt.m_double);
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%04d-%02d-%02dT%02d:%02d:%02dZ",
                  tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                  tm.tm_hour, tm.tm_min, tm.tm_sec);
    return buf;
}

// Parses a feed date string into CF days. Handles the W3C
// ("2020-01-06T12:00:00Z" / "+hh:mm") and RFC822
// ("Mon, 06 Jan 2020 12:00:00 GMT") formats Rome's DateParser accepts, then
// falls back to the engine's general date parser. A timezone offset shifts the
// stored wall-clock so the W3C/RFC822 re-render matches CF's GMT-based output.
bool feedParseDate(const std::string &s, double &days)
{
    std::string str = s;
    // Trim.
    size_t b = str.find_first_not_of(" \t\r\n");
    size_t e = str.find_last_not_of(" \t\r\n");
    if (b == std::string::npos) return false;
    str = str.substr(b, e - b + 1);
    if (str.empty()) return false;

    int year = 0, mon = 0, mday = 0, hour = 0, min = 0, sec = 0;
    bool matched = false;
    // W3C: yyyy-MM-ddTHH:mm:ss[Z|±hh:mm]
    {
        int n = 0;
        if (std::sscanf(str.c_str(), "%d-%d-%dT%d:%d:%d%n", &year, &mon, &mday, &hour, &min, &sec, &n) >= 6) {
            std::string rest = str.substr(static_cast<size_t>(n));
            int offSecs = 0;
            if (!rest.empty() && rest[0] == 'Z') {
                // UTC.
            } else if (!rest.empty() && (rest[0] == '+' || rest[0] == '-')) {
                int oh = 0, om = 0;
                if (std::sscanf(rest.c_str() + 1, "%d:%d", &oh, &om) >= 1) {
                    offSecs = oh * 3600 + om * 60;
                    if (rest[0] == '-') offSecs = -offSecs;
                }
            }
            // Convert the offset to the local wall-clock: CF's Rome
            // DateParser.parseDate + formatW3CDateTime render the instant in
            // GMT, so store (components - offset).
            struct tm tm;
            memset(&tm, 0, sizeof(tm));
            tm.tm_year = year - 1900; tm.tm_mon = mon - 1; tm.tm_mday = mday;
            tm.tm_hour = hour; tm.tm_min = min; tm.tm_sec = sec;
            time_t utc = timegm(&tm);
            utc -= offSecs;
            struct tm gmt;
            memset(&gmt, 0, sizeof(gmt));
            gmtime_r(&utc, &gmt);
            days = cfml::tmToDays(gmt);
            matched = true;
        }
    }
    if (!matched) {
        // RFC822: EEE, dd MMM yyyy HH:mm:ss TZ
        static const char *months[] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
        int comma = -1;
        for (size_t i = 0; i < str.size(); i++) if (str[i] == ',') { comma = static_cast<int>(i); break; }
        if (comma >= 0) {
            std::string rest = str.substr(static_cast<size_t>(comma) + 1);
            size_t rb = rest.find_first_not_of(" \t");
            if (rb != std::string::npos) rest = rest.substr(rb);
            int dd = 0, mIdx = -1, yy = 0, hh = 0, mm = 0, ss = 0;
            char monName[8] = {0};
            char tz[8] = {0};
            int n = 0;
            if (std::sscanf(rest.c_str(), "%d %3s %d %d:%d:%d %3s%n", &dd, monName, &yy, &hh, &mm, &ss, tz, &n) >= 6) {
                for (int i = 0; i < 12; i++) if (strcasecmp(monName, months[i]) == 0) { mIdx = i; break; }
                if (mIdx >= 0) {
                    int offSecs = 0;
                    std::string tzs(tz);
                    if (tzs == "GMT" || tzs == "UTC" || tzs == "UT" || tzs == "Z") offSecs = 0;
                    else if (tzs == "EST") offSecs = 5 * 3600;
                    else if (tzs == "EDT") offSecs = 4 * 3600;
                    else if (tzs == "CST") offSecs = 6 * 3600;
                    else if (tzs == "CDT") offSecs = 5 * 3600;
                    else if (tzs == "MST") offSecs = 7 * 3600;
                    else if (tzs == "MDT") offSecs = 6 * 3600;
                    else if (tzs == "PST") offSecs = 8 * 3600;
                    else if (tzs == "PDT") offSecs = 7 * 3600;
                    struct tm tm;
                    memset(&tm, 0, sizeof(tm));
                    tm.tm_year = yy - 1900; tm.tm_mon = mIdx; tm.tm_mday = dd;
                    tm.tm_hour = hh; tm.tm_min = mm; tm.tm_sec = ss;
                    time_t utc = timegm(&tm);
                    utc -= offSecs;
                    struct tm gmt;
                    memset(&gmt, 0, sizeof(gmt));
                    gmtime_r(&utc, &gmt);
                    days = cfml::tmToDays(gmt);
                    matched = true;
                }
            }
        }
    }
    if (!matched) {
        // Engine's general date parser (CF date formats).
        if (cfml::parseDateTimeStr(str.c_str(), days)) matched = true;
    }
    return matched;
}

// Parses a feed date value: a DateTime passes through, a string is parsed.
bool feedDateFromValue(const cfvariant &v, double &days)
{
    if (v.m_type == cfvariant::DateTime) { days = v.m_double; return true; }
    return feedParseDate(cfml::safe_to_std_string(v), days);
}

// ---- JDOM pretty-print (WireFeedOutput's Format.getPrettyFormat) ----

struct XmlEl {
    std::string name;
    std::vector<std::pair<std::string, std::string>> attrs;
    std::vector<XmlEl> children;
    std::string text;
    bool hasText = false;

    void attr(const std::string &k, const std::string &v) { attrs.push_back({k, v}); }
    void child(XmlEl &&c) { children.push_back(std::move(c)); }
};

std::string escapeXmlText(const std::string &s)
{
    std::string out;
    for (char c : s) {
        if (c == '&') out += "&amp;";
        else if (c == '<') out += "&lt;";
        else if (c == '>') out += "&gt;";
        else if (c == '\r') out += "&#xD;";
        else out += c;
    }
    return out;
}

std::string escapeXmlAttr(const std::string &s)
{
    std::string out;
    for (char c : s) {
        if (c == '&') out += "&amp;";
        else if (c == '<') out += "&lt;";
        else if (c == '>') out += "&gt;";
        else if (c == '"') out += "&quot;";
        else if (c == '\n') out += "&#xA;";
        else if (c == '\r') out += "&#xD;";
        else if (c == '\t') out += "&#x9;";
        else out += c;
    }
    return out;
}

void printXmlEl(const XmlEl &el, int depth, std::string &out)
{
    // Rome's WireFeedOutput pretty-print uses CRLF line separators (JDOM
    // Format.getPrettyFormat with the default system line separator on CF).
    static const std::string nl = "\r\n";
    out.append(static_cast<size_t>(depth) * 2, ' ');
    out += "<" + el.name;
    for (const auto &a : el.attrs) out += " " + a.first + "=\"" + escapeXmlAttr(a.second) + "\"";
    if (el.hasText) {
        out += ">" + escapeXmlText(el.text) + "</" + el.name + ">" + nl;
    } else if (el.children.empty()) {
        out += "/>" + nl;
    } else {
        out += ">" + nl;
        for (const auto &c : el.children) printXmlEl(c, depth + 1, out);
        out.append(static_cast<size_t>(depth) * 2, ' ');
        out += "</" + el.name + ">" + nl;
    }
}

// The XML-element equivalent of insertNotNull.
void insertNotNull(XmlEl &el, const char *key, const std::string &value)
{
    if (!value.empty()) {
        XmlEl c;
        c.name = key;
        c.text = value;
        c.hasText = true;
        el.child(std::move(c));
    }
}

// ---- feed source reading ----

size_t feedCurlCb(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    std::string *buf = static_cast<std::string*>(userdata);
    size_t n = size * nmemb;
    buf->append(ptr, n);
    return n;
}

bool readFeedSource(const std::string &source, std::string &xml, const std::string &userAgent, long timeoutSecs)
{
    if (source.rfind("http://", 0) == 0 || source.rfind("https://", 0) == 0) {
        CURL *curl = curl_easy_init();
        if (!curl) return false;
        std::string headers;
        curl_easy_setopt(curl, CURLOPT_URL, source.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, feedCurlCb);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &xml);
        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, feedCurlCb);
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, &headers);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, userAgent.c_str());
        if (timeoutSecs > 0) curl_easy_setopt(curl, CURLOPT_TIMEOUT, static_cast<long>(timeoutSecs));
        curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "deflate;q=0");
        CURLcode rc = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        if (rc != CURLE_OK) return false;
        return true;
    }
    // Local file.
    std::ifstream in(source, std::ios::binary);
    if (!in.is_open()) return false;
    std::stringstream ss;
    ss << in.rdbuf();
    xml = ss.str();
    return true;
}

// ---- helpers to extract a child element's text / a direct child element ----

std::string childText(xmlNodePtr parent, const char *name)
{
    for (xmlNodePtr ch = parent->children; ch; ch = ch->next) {
        if (ch->type == XML_ELEMENT_NODE && xmlStrcmp(ch->name, BAD_CAST name) == 0) {
            xmlChar *c = xmlNodeGetContent(ch);
            std::string res = c ? reinterpret_cast<const char*>(c) : "";
            if (c) xmlFree(c);
            return res;
        }
    }
    return "";
}

std::string childAttr(xmlNodePtr parent, const char *name, const char *attr)
{
    for (xmlNodePtr ch = parent->children; ch; ch = ch->next) {
        if (ch->type == XML_ELEMENT_NODE && xmlStrcmp(ch->name, BAD_CAST name) == 0) {
            xmlChar *c = xmlGetProp(ch, BAD_CAST attr);
            std::string res = c ? reinterpret_cast<const char*>(c) : "";
            if (c) xmlFree(c);
            return res;
        }
    }
    return "";
}

// Reads an attribute on a node, freeing the libxml2 allocation (xmlGetProp
// returns a malloc'd string that must be xmlFree'd).
std::string propText(xmlNodePtr node, const char *attr)
{
    xmlChar *p = xmlGetProp(node, BAD_CAST attr);
    std::string res = p ? reinterpret_cast<const char*>(p) : "";
    if (p) xmlFree(p);
    return res;
}

xmlNodePtr findChild(xmlNodePtr parent, const char *name)
{
    for (xmlNodePtr ch = parent->children; ch; ch = ch->next) {
        if (ch->type == XML_ELEMENT_NODE && xmlStrcmp(ch->name, BAD_CAST name) == 0) return ch;
    }
    return nullptr;
}

std::vector<xmlNodePtr> childrenNamed(xmlNodePtr parent, const char *name)
{
    std::vector<xmlNodePtr> res;
    for (xmlNodePtr ch = parent->children; ch; ch = ch->next) {
        if (ch->type == XML_ELEMENT_NODE && xmlStrcmp(ch->name, BAD_CAST name) == 0) res.push_back(ch);
    }
    return res;
}

// The comma-joined RSS date list rendering for dc: elements.
std::string listJoin(const std::vector<std::string> &vals)
{
    std::string out;
    for (size_t i = 0; i < vals.size(); i++) {
        if (i) out += ",";
        out += vals[i];
    }
    return out;
}

// Split a comma-delimited string; blank tokens skipped.
std::vector<std::string> splitList(const std::string &s)
{
    std::vector<std::string> out;
    std::string cur;
    for (size_t i = 0; i <= s.size(); i++) {
        if (i == s.size() || s[i] == ',') {
            if (!cur.empty()) out.push_back(cur);
            cur.clear();
        } else {
            cur += s[i];
        }
    }
    return out;
}

} // namespace

namespace cfml {

// ---- RSS read ----

namespace {

void rssMetadataStruct(xmlNodePtr channel, const std::string &version, const std::string &encoding,
                       cfvariant &md)
{
    insertNotNull(md, "version", version);
    if (!encoding.empty()) insertNotNull(md, "encoding", encoding);
    insertNotNull(md, "title", childText(channel, "title"));
    insertNotNull(md, "link", childText(channel, "link"));
    insertNotNull(md, "description", childText(channel, "description"));
    insertNotNull(md, "language", childText(channel, "language"));
    insertNotNull(md, "copyright", childText(channel, "copyright"));
    insertNotNull(md, "managingEditor", childText(channel, "managingEditor"));
    insertNotNull(md, "webMaster", childText(channel, "webMaster"));
    std::string pubDate = childText(channel, "pubDate");
    if (!pubDate.empty()) md.set("pubDate") = cfvariant(pubDate.c_str());
    std::string lastBuild = childText(channel, "lastBuildDate");
    if (!lastBuild.empty()) md.set("lastBuildDate") = cfvariant(lastBuild.c_str());
    insertNotNull(md, "generator", childText(channel, "generator"));
    insertNotNull(md, "docs", childText(channel, "docs"));
    std::string ttl = childText(channel, "ttl");
    if (!ttl.empty()) {
        cfvariant t(cfvariant::Number);
        t.m_int = std::atoi(ttl.c_str());
        md.set("ttl") = t;
    }
    // category array
    std::vector<xmlNodePtr> cats = childrenNamed(channel, "category");
    if (!cats.empty()) {
        cfvariant catArr(cfvariant::Array);
        if (!catArr.m_array) catArr.m_array = new std::vector<cfvariant>();
        for (xmlNodePtr c : cats) {
            cfvariant cs(cfvariant::Struct);
            std::string domain = childAttr(channel, "category", "domain");
            std::string value = childText(c, "category");
            if (value.empty()) {
                xmlChar *content = xmlNodeGetContent(c);
                if (content) { value = reinterpret_cast<const char*>(content); xmlFree(content); }
            }
            // category has no child elements; value is its text
            xmlChar *content = xmlNodeGetContent(c);
            std::string cv = content ? reinterpret_cast<const char*>(content) : "";
            if (content) xmlFree(content);
            insertNotNull(cs, "domain", domain);
            insertNotNull(cs, "value", cv.empty() ? value : cv);
            if (!cs.m_struct || cs.m_struct->empty()) continue;
            catArr.insert(cs);
        }
        if (!catArr.m_array || !catArr.m_array->empty()) md.set("category") = catArr;
    }
    // image
    xmlNodePtr image = findChild(channel, "image");
    if (image) {
        cfvariant img(cfvariant::Struct);
        insertNotNull(img, "url", childText(image, "url"));
        insertNotNull(img, "title", childText(image, "title"));
        insertNotNull(img, "link", childText(image, "link"));
        std::string w = childText(image, "width");
        if (!w.empty()) { cfvariant v(cfvariant::Number); v.m_int = std::atoi(w.c_str()); img.set("width") = v; }
        std::string h = childText(image, "height");
        if (!h.empty()) { cfvariant v(cfvariant::Number); v.m_int = std::atoi(h.c_str()); img.set("height") = v; }
        insertNotNull(img, "description", childText(image, "description"));
        if (img.m_struct && !img.m_struct->empty()) md.set("image") = img;
    }
    insertNotNull(md, "rating", childText(channel, "rating"));
    // textInput
    xmlNodePtr textInput = findChild(channel, "textInput");
    if (textInput) {
        cfvariant ti(cfvariant::Struct);
        insertNotNull(ti, "title", childText(textInput, "title"));
        insertNotNull(ti, "description", childText(textInput, "description"));
        insertNotNull(ti, "name", childText(textInput, "name"));
        insertNotNull(ti, "link", childText(textInput, "link"));
        if (ti.m_struct && !ti.m_struct->empty()) md.set("textInput") = ti;
    }
    // skipHours / skipDays
    xmlNodePtr skipHours = findChild(channel, "skipHours");
    if (skipHours) {
        std::vector<std::string> hours;
        for (xmlNodePtr h = skipHours->children; h; h = h->next) {
            if (h->type == XML_ELEMENT_NODE) {
                xmlChar *c = xmlNodeGetContent(h);
                if (c) { hours.push_back(reinterpret_cast<const char*>(c)); xmlFree(c); }
            }
        }
        insertNotNull(md, "skipHours", listJoin(hours));
    }
    xmlNodePtr skipDays = findChild(channel, "skipDays");
    if (skipDays) {
        std::vector<std::string> days;
        for (xmlNodePtr d = skipDays->children; d; d = d->next) {
            if (d->type == XML_ELEMENT_NODE) {
                xmlChar *c = xmlNodeGetContent(d);
                if (c) { days.push_back(reinterpret_cast<const char*>(c)); xmlFree(c); }
            }
        }
        insertNotNull(md, "skipDays", listJoin(days));
    }
    // cloud
    xmlNodePtr cloud = findChild(channel, "cloud");
    if (cloud) {
        cfvariant cs(cfvariant::Struct);
        insertNotNull(cs, "domain", childAttr(channel, "cloud", "domain"));
        std::string port = childAttr(channel, "cloud", "port");
        if (!port.empty()) { cfvariant v(cfvariant::Number); v.m_int = std::atoi(port.c_str()); cs.set("port") = v; }
        insertNotNull(cs, "path", childAttr(channel, "cloud", "path"));
        insertNotNull(cs, "protocol", childAttr(channel, "cloud", "protocol"));
        insertNotNull(cs, "registerProcedure", childAttr(channel, "cloud", "registerProcedure"));
        if (cs.m_struct && !cs.m_struct->empty()) md.set("cloud") = cs;
    }
}

// One RSS <item> into the item struct. rssVersion >= 0.94 renders the
// description as a {type,value} struct (CF's RSSParser.getItemAsArray).
cfvariant rssItemStruct(xmlNodePtr item, double rssVersion)
{
    cfvariant st(cfvariant::Struct);
    insertNotNull(st, "title", childText(item, "title"));
    xmlNodePtr desc = findChild(item, "description");
    if (desc) {
        xmlChar *content = xmlNodeGetContent(desc);
        std::string value = content ? reinterpret_cast<const char*>(content) : "";
        if (content) xmlFree(content);
        if (rssVersion < 0.94) {
            insertNotNull(st, "description", value);
        } else {
            cfvariant ds(cfvariant::Struct);
            std::string type = propText(desc, "type");
            insertNotNull(ds, "type", type);
            insertNotNull(ds, "value", value);
            if (ds.m_struct && !ds.m_struct->empty()) st.set("description") = ds;
        }
    }
    insertNotNull(st, "link", childText(item, "link"));
    insertNotNull(st, "uri", childText(item, "uri"));
    xmlNodePtr source = findChild(item, "source");
    if (source) {
        cfvariant ss(cfvariant::Struct);
        std::string url = propText(source, "url");
        xmlChar *content = xmlNodeGetContent(source);
        std::string value = content ? reinterpret_cast<const char*>(content) : "";
        if (content) xmlFree(content);
        insertNotNull(ss, "value", value);
        insertNotNull(ss, "url", url);
        if (ss.m_struct && !ss.m_struct->empty()) st.set("source") = ss;
    }
    xmlNodePtr guid = findChild(item, "guid");
    if (guid) {
        cfvariant gs(cfvariant::Struct);
        xmlChar *content = xmlNodeGetContent(guid);
        std::string value = content ? reinterpret_cast<const char*>(content) : "";
        if (content) xmlFree(content);
        insertNotNull(gs, "value", value);
        std::string permalink = propText(guid, "isPermaLink");
        if (permalink.empty()) permalink = "true";
        cfvariant b(cfvariant::Boolean);
        b.m_bool = (permalink == "true");
        gs.set("isPermaLink") = b;
        if (!gs.m_struct || gs.m_struct->empty()) insertNotNull(gs, "value", value);
        st.set("guid") = gs;
    }
    // enclosures
    std::vector<xmlNodePtr> encs = childrenNamed(item, "enclosure");
    if (!encs.empty()) {
        cfvariant arr(cfvariant::Array);
        if (!arr.m_array) arr.m_array = new std::vector<cfvariant>();
        for (xmlNodePtr e : encs) {
            cfvariant es(cfvariant::Struct);
            insertNotNull(es, "url", childAttr(item, "enclosure", "url"));
            std::string len = childAttr(item, "enclosure", "length");
            if (!len.empty()) { cfvariant v(cfvariant::Long); v.m_long = std::atoll(len.c_str()); es.set("length") = v; }
            insertNotNull(es, "type", childAttr(item, "enclosure", "type"));
            if (es.m_struct && !es.m_struct->empty()) arr.insert(es);
        }
        if (!arr.m_array || !arr.m_array->empty()) st.set("enclosure") = arr;
    }
    // categories
    std::vector<xmlNodePtr> cats = childrenNamed(item, "category");
    if (!cats.empty()) {
        cfvariant arr(cfvariant::Array);
        if (!arr.m_array) arr.m_array = new std::vector<cfvariant>();
        for (xmlNodePtr c : cats) {
            cfvariant cs(cfvariant::Struct);
            std::string domain = propText(c, "domain");
            xmlChar *content = xmlNodeGetContent(c);
            std::string value = content ? reinterpret_cast<const char*>(content) : "";
            if (content) xmlFree(content);
            insertNotNull(cs, "domain", domain);
            insertNotNull(cs, "value", value);
            if (cs.m_struct && !cs.m_struct->empty()) arr.insert(cs);
        }
        if (!arr.m_array || !arr.m_array->empty()) st.set("category") = arr;
    }
    insertNotNull(st, "comments", childText(item, "comments"));
    insertNotNull(st, "author", childText(item, "author"));
    std::string pubDate = childText(item, "pubDate");
    if (!pubDate.empty()) st.set("pubDate") = cfvariant(pubDate.c_str());
    std::string expDate = childText(item, "expirationDate");
    if (!expDate.empty()) st.set("expirationDate") = cfvariant(expDate.c_str());
    return st;
}

// The RSS entries query: FeedTable's base column set (all VARCHAR). One row per
// item with the mapped values.
void rssItemsQuery(xmlNodePtr channel, double rssVersion, cfvariant &q)
{
    webstrada::QueryData *qd = q.m_query;
    static const char *cols[] = {
        "TITLE","TITLETYPE","PUBLISHEDDATE","EXPIRATIONDATE","COMMENTS","CREATEDDATE",
        "UPDATEDDATE","RIGHTS","SOURCE","SOURCEURL","CATEGORYSCHEME","CATEGORYLABEL",
        "CATEGORYTERM","LINKREL","LINKHREF","LINKLENGTH","LINKTYPE","LINKHREFLANG",
        "LINKTITLE","RSSLINK","AUTHORNAME","AUTHORURI","AUTHOREMAIL","CONTRIBUTORNAME",
        "CONTRIBUTORURI","CONTRIBUTOREMAIL","ID","IDPERMALINK","CONTENT","CONTENTTYPE",
        "CONTENTSRC","CONTENTMODE","URI","XMLBASE","SUMMARY","SUMMARYTYPE","SUMMARYSRC",
        "SUMMARYMODE"
    };
    for (const char *c : cols) {
        webstrada::QueryColumn col;
        col.name = c;
        col.type = "VARCHAR";
        qd->columns.push_back(std::move(col));
    }
    auto setCell = [&](int row, const char *col, const std::string &v) {
        for (size_t ci = 0; ci < qd->columns.size(); ci++) {
            if (qd->columns[ci].name.compareCaseInsensitive(col) == 0) {
                while (static_cast<int>(qd->columns[ci].values.size()) <= row) qd->columns[ci].values.emplace_back(cfvariant(cfvariant::Null));
                qd->columns[ci].values[row] = cfvariant(v.c_str());
                return;
            }
        }
    };
    std::vector<xmlNodePtr> items = childrenNamed(channel, "item");
    int row = 0;
    for (xmlNodePtr item : items) {
        xmlNodePtr desc = findChild(item, "description");
        xmlChar *descContent = desc ? xmlNodeGetContent(desc) : nullptr;
        std::string descVal = descContent ? reinterpret_cast<const char*>(descContent) : "";
        if (descContent) xmlFree(descContent);
        setCell(row, "TITLE", childText(item, "title"));
        if (desc && rssVersion >= 0.94) {
            std::string t = propText(desc, "type");
            setCell(row, "CONTENTTYPE", t);
        }
        setCell(row, "CONTENT", descVal);
        setCell(row, "RSSLINK", childText(item, "link"));
        setCell(row, "URI", childText(item, "uri"));
        xmlNodePtr source = findChild(item, "source");
        if (source) {
            std::string url = propText(source, "url");
            xmlChar *content = xmlNodeGetContent(source);
            std::string sv = content ? reinterpret_cast<const char*>(content) : "";
            if (content) xmlFree(content);
            setCell(row, "SOURCE", sv);
            setCell(row, "SOURCEURL", url);
        }
        xmlNodePtr guid = findChild(item, "guid");
        if (guid) {
            xmlChar *content = xmlNodeGetContent(guid);
            std::string gv = content ? reinterpret_cast<const char*>(content) : "";
            if (content) xmlFree(content);
            setCell(row, "ID", gv);
            std::string pl = propText(guid, "isPermaLink");
            if (pl.empty()) pl = "true";
            setCell(row, "IDPERMALINK", pl);
        }
        std::vector<std::string> hrefs, lens, types;
        for (xmlNodePtr e : childrenNamed(item, "enclosure")) {
            std::string url = propText(e, "url");
            std::string len = propText(e, "length");
            std::string typ = propText(e, "type");
            hrefs.push_back(url.empty() ? " " : url);
            lens.push_back(len.empty() ? " " : len);
            types.push_back(typ.empty() ? " " : typ);
        }
        if (!hrefs.empty()) {
            setCell(row, "LINKHREF", listJoin(hrefs));
            setCell(row, "LINKLENGTH", listJoin(lens));
            setCell(row, "LINKTYPE", listJoin(types));
        }
        std::vector<std::string> catVals, catDomains;
        for (xmlNodePtr c : childrenNamed(item, "category")) {
            std::string domain = propText(c, "domain");
            xmlChar *content = xmlNodeGetContent(c);
            std::string v = content ? reinterpret_cast<const char*>(content) : "";
            if (content) xmlFree(content);
            catDomains.push_back(domain.empty() ? " " : domain);
            catVals.push_back(v.empty() ? " " : v);
        }
        if (!catVals.empty()) {
            setCell(row, "CATEGORYSCHEME", listJoin(catDomains));
            setCell(row, "CATEGORYLABEL", listJoin(catVals));
        }
        setCell(row, "COMMENTS", childText(item, "comments"));
        setCell(row, "AUTHOREMAIL", childText(item, "author"));
        setCell(row, "PUBLISHEDDATE", childText(item, "pubDate"));
        setCell(row, "EXPIRATIONDATE", childText(item, "expirationDate"));
        row++;
    }
    qd->m_rowCount = row;
}

// ---- Atom read ----

// Reads an atom <content> element (type/src/value) into a CF struct.
cfvariant atomContentStruct(xmlNodePtr node)
{
    cfvariant st(cfvariant::Struct);
    std::string type = propText(node, "type");
    std::string src = propText(node, "src");
    xmlChar *content = xmlNodeGetContent(node);
    std::string value = content ? reinterpret_cast<const char*>(content) : "";
    if (content) xmlFree(content);
    insertNotNull(st, "type", type);
    insertNotNull(st, "src", src);
    insertNotNull(st, "value", value);
    return st;
}

void atomMetadataStruct(xmlNodePtr feed, const std::string &version, const std::string &encoding,
                        cfvariant &md)
{
    insertNotNull(md, "version", version);
    if (!encoding.empty()) insertNotNull(md, "encoding", encoding);
    xmlNodePtr title = findChild(feed, "title");
    if (title) md.set("title") = atomContentStruct(title);
    insertNotNull(md, "link", childText(feed, "link"));
    xmlNodePtr subtitle = findChild(feed, "subtitle");
    if (subtitle) md.set("subtitle") = atomContentStruct(subtitle);
    insertNotNull(md, "id", childText(feed, "id"));
    insertNotNull(md, "updated", childText(feed, "updated"));
    insertNotNull(md, "rights", childText(feed, "rights"));
    insertNotNull(md, "icon", childText(feed, "icon"));
    insertNotNull(md, "logo", childText(feed, "logo"));
    // authors
    std::vector<xmlNodePtr> authors = childrenNamed(feed, "author");
    if (!authors.empty()) {
        cfvariant arr(cfvariant::Array);
        if (!arr.m_array) arr.m_array = new std::vector<cfvariant>();
        for (xmlNodePtr a : authors) {
            cfvariant as(cfvariant::Struct);
            insertNotNull(as, "name", childText(a, "name"));
            insertNotNull(as, "uri", childText(a, "uri"));
            insertNotNull(as, "email", childText(a, "email"));
            if (as.m_struct && !as.m_struct->empty()) arr.insert(as);
        }
        if (!arr.m_array || !arr.m_array->empty()) md.set("author") = arr;
    }
    // links (as an array of structs, CF's AtomParser.getMetadataAsStruct)
    std::vector<xmlNodePtr> links = childrenNamed(feed, "link");
    if (!links.empty()) {
        cfvariant arr(cfvariant::Array);
        if (!arr.m_array) arr.m_array = new std::vector<cfvariant>();
        for (xmlNodePtr l : links) {
            cfvariant ls(cfvariant::Struct);
            insertNotNull(ls, "href", childAttr(feed, "link", "href"));
            insertNotNull(ls, "rel", childAttr(feed, "link", "rel"));
            insertNotNull(ls, "type", childAttr(feed, "link", "type"));
            insertNotNull(ls, "hreflang", childAttr(feed, "link", "hreflang"));
            insertNotNull(ls, "title", childAttr(feed, "link", "title"));
            std::string len = childAttr(feed, "link", "length");
            if (!len.empty()) { cfvariant v(cfvariant::Long); v.m_long = std::atoll(len.c_str()); ls.set("length") = v; }
            if (ls.m_struct && !ls.m_struct->empty()) arr.insert(ls);
        }
        if (!arr.m_array || !arr.m_array->empty()) md.set("link") = arr;
    }
}

cfvariant atomEntryStruct(xmlNodePtr entry)
{
    cfvariant st(cfvariant::Struct);
    xmlNodePtr title = findChild(entry, "title");
    if (title) st.set("title") = atomContentStruct(title);
    insertNotNull(st, "id", childText(entry, "id"));
    insertNotNull(st, "updated", childText(entry, "updated"));
    insertNotNull(st, "published", childText(entry, "published"));
    insertNotNull(st, "created", childText(entry, "created"));
    insertNotNull(st, "rights", childText(entry, "rights"));
    xmlNodePtr summary = findChild(entry, "summary");
    if (summary) st.set("summary") = atomContentStruct(summary);
    xmlNodePtr content = findChild(entry, "content");
    if (content) {
        cfvariant arr(cfvariant::Array);
        if (!arr.m_array) arr.m_array = new std::vector<cfvariant>();
        arr.insert(atomContentStruct(content));
        st.set("content") = arr;
    }
    std::vector<xmlNodePtr> authors = childrenNamed(entry, "author");
    if (!authors.empty()) {
        cfvariant arr(cfvariant::Array);
        if (!arr.m_array) arr.m_array = new std::vector<cfvariant>();
        for (xmlNodePtr a : authors) {
            cfvariant as(cfvariant::Struct);
            insertNotNull(as, "name", childText(a, "name"));
            insertNotNull(as, "uri", childText(a, "uri"));
            insertNotNull(as, "email", childText(a, "email"));
            if (as.m_struct && !as.m_struct->empty()) arr.insert(as);
        }
        if (!arr.m_array || !arr.m_array->empty()) st.set("author") = arr;
    }
    std::vector<xmlNodePtr> categories = childrenNamed(entry, "category");
    if (!categories.empty()) {
        cfvariant arr(cfvariant::Array);
        if (!arr.m_array) arr.m_array = new std::vector<cfvariant>();
        for (xmlNodePtr c : categories) {
            cfvariant cs(cfvariant::Struct);
            insertNotNull(cs, "term", childAttr(entry, "category", "term"));
            insertNotNull(cs, "scheme", childAttr(entry, "category", "scheme"));
            insertNotNull(cs, "label", childAttr(entry, "category", "label"));
            if (cs.m_struct && !cs.m_struct->empty()) arr.insert(cs);
        }
        if (!arr.m_array || !arr.m_array->empty()) st.set("category") = arr;
    }
    std::vector<xmlNodePtr> links = childrenNamed(entry, "link");
    if (!links.empty()) {
        cfvariant arr(cfvariant::Array);
        if (!arr.m_array) arr.m_array = new std::vector<cfvariant>();
        for (xmlNodePtr l : links) {
            cfvariant ls(cfvariant::Struct);
            insertNotNull(ls, "href", childAttr(entry, "link", "href"));
            insertNotNull(ls, "rel", childAttr(entry, "link", "rel"));
            insertNotNull(ls, "type", childAttr(entry, "link", "type"));
            insertNotNull(ls, "hreflang", childAttr(entry, "link", "hreflang"));
            insertNotNull(ls, "title", childAttr(entry, "link", "title"));
            if (ls.m_struct && !ls.m_struct->empty()) arr.insert(ls);
        }
        if (!arr.m_array || !arr.m_array->empty()) st.set("link") = arr;
    }
    return st;
}

// ---- RSS 2.0 generation ----

std::string rssDateFromValue(const cfvariant &st, const char *key)
{
    const cfvariant *v = attrOf(&st, key);
    if (!v) return "";
    double days = 0;
    if (!feedDateFromValue(*v, days)) return "";
    cfvariant dt(cfvariant::DateTime);
    dt.m_double = days;
    return formatRFC822(dt);
}

// Atom dates render with the W3C format ("yyyy-MM-dd'T'HH:mm:ss'Z'", GMT).
std::string atomDateFromValue(const cfvariant &st, const char *key)
{
    const cfvariant *v = attrOf(&st, key);
    if (!v) return "";
    double days = 0;
    if (!feedDateFromValue(*v, days)) return "";
    cfvariant dt(cfvariant::DateTime);
    dt.m_double = days;
    return formatW3CDateTime(dt);
}

std::string structOrString(const cfvariant &st, const char *key, const char *sub)
{
    const cfvariant *v = attrOf(&st, key);
    if (!v) return "";
    if (v->m_type == cfvariant::Struct) return stGet(*v, sub);
    std::string s = cfml::safe_to_std_string(*v);
    if (s == " ") return "";
    return s;
}

XmlEl rssSimpleEl(const char *name, const std::string &value)
{
    XmlEl el;
    el.name = name;
    el.text = value;
    el.hasText = !value.empty();
    return el;
}

XmlEl buildRssFeedXml(const cfvariant &feedStruct, const std::string &encoding)
{
    XmlEl rss;
    rss.name = "rss";
    rss.attr("version", "2.0");
    XmlEl channel;
    channel.name = "channel";
    insertNotNull(channel, "title", stGet(feedStruct, "title"));
    insertNotNull(channel, "link", stGet(feedStruct, "link"));
    insertNotNull(channel, "description", stGet(feedStruct, "description"));
    insertNotNull(channel, "language", stGet(feedStruct, "language"));
    insertNotNull(channel, "rating", stGet(feedStruct, "rating"));
    insertNotNull(channel, "copyright", stGet(feedStruct, "copyright"));
    std::string pubDate = rssDateFromValue(feedStruct, "pubDate");
    if (!pubDate.empty()) channel.child(rssSimpleEl("pubDate", pubDate));
    std::string lastBuild = rssDateFromValue(feedStruct, "lastBuildDate");
    if (!lastBuild.empty()) channel.child(rssSimpleEl("lastBuildDate", lastBuild));
    insertNotNull(channel, "docs", stGet(feedStruct, "docs"));
    insertNotNull(channel, "managingEditor", stGet(feedStruct, "managingeditor"));
    insertNotNull(channel, "webMaster", stGet(feedStruct, "webMaster"));
    std::string skipHours = stGet(feedStruct, "skipHours");
    if (!skipHours.empty()) {
        XmlEl sh;
        sh.name = "skipHours";
        for (auto &h : splitList(skipHours)) sh.child(rssSimpleEl("hour", h));
        channel.child(std::move(sh));
    }
    std::string skipDays = stGet(feedStruct, "skipDays");
    if (!skipDays.empty()) {
        XmlEl sd;
        sd.name = "skipDays";
        for (auto &d : splitList(skipDays)) sd.child(rssSimpleEl("day", d));
        channel.child(std::move(sd));
    }
    // cloud
    const cfvariant *cloud = attrOf(&feedStruct, "cloud");
    if (cloud && cloud->m_type == cfvariant::Struct) {
        XmlEl c;
        c.name = "cloud";
        std::string domain = stGet(*cloud, "domain");
        if (!domain.empty()) c.attr("domain", domain);
        std::string port = stGet(*cloud, "port");
        if (!port.empty() && port != "0") c.attr("port", port);
        std::string path = stGet(*cloud, "path");
        if (!path.empty()) c.attr("path", path);
        std::string registerProcedure = stGet(*cloud, "registerProcedure");
        if (!registerProcedure.empty()) c.attr("registerProcedure", registerProcedure);
        std::string protocol = stGet(*cloud, "protocol");
        if (!protocol.empty()) c.attr("protocol", protocol);
        channel.child(std::move(c));
    }
    insertNotNull(channel, "generator", stGet(feedStruct, "generator"));
    std::string ttl = stGet(feedStruct, "ttl");
    if (!ttl.empty()) channel.child(rssSimpleEl("ttl", ttl));
    // categories
    const cfvariant *cats = attrOf(&feedStruct, "category");
    if (cats && cats->m_type == cfvariant::Array && cats->m_array) {
        for (const auto &cat : *cats->m_array) {
            if (cat.m_type != cfvariant::Struct) continue;
            XmlEl c;
            c.name = "category";
            std::string domain = stGet(cat, "domain");
            if (!domain.empty()) c.attr("domain", domain);
            c.text = stGet(cat, "value");
            c.hasText = !c.text.empty();
            channel.child(std::move(c));
        }
    }
    // image
    const cfvariant *image = attrOf(&feedStruct, "image");
    if (image && image->m_type == cfvariant::Struct) {
        XmlEl im;
        im.name = "image";
        insertNotNull(im, "url", stGet(*image, "url"));
        insertNotNull(im, "title", stGet(*image, "title"));
        insertNotNull(im, "link", stGet(*image, "link"));
        std::string w = stGet(*image, "width");
        if (!w.empty()) im.child(rssSimpleEl("width", w));
        std::string h = stGet(*image, "height");
        if (!h.empty()) im.child(rssSimpleEl("height", h));
        insertNotNull(im, "description", stGet(*image, "description"));
        channel.child(std::move(im));
    }
    // textInput
    const cfvariant *textInput = attrOf(&feedStruct, "textInput");
    if (textInput && textInput->m_type == cfvariant::Struct) {
        XmlEl ti;
        ti.name = "textInput";
        insertNotNull(ti, "title", stGet(*textInput, "title"));
        insertNotNull(ti, "description", stGet(*textInput, "description"));
        insertNotNull(ti, "name", stGet(*textInput, "name"));
        insertNotNull(ti, "link", stGet(*textInput, "link"));
        channel.child(std::move(ti));
    }
    // items
    const cfvariant *items = attrOf(&feedStruct, "item");
    if (items && items->m_type == cfvariant::Array && items->m_array) {
        for (const auto &item : *items->m_array) {
            if (item.m_type != cfvariant::Struct) continue;
            XmlEl it;
            it.name = "item";
            insertNotNull(it, "title", stGet(item, "title"));
            insertNotNull(it, "link", stGet(item, "link"));
            std::string descValue = structOrString(item, "description", "value");
            if (!descValue.empty()) {
                XmlEl d;
                d.name = "description";
                d.text = descValue;
                d.hasText = true;
                it.child(std::move(d));
            }
            const cfvariant *source = attrOf(&item, "source");
            if (source && source->m_type == cfvariant::Struct) {
                XmlEl s;
                s.name = "source";
                std::string url = stGet(*source, "url");
                if (!url.empty()) s.attr("url", url);
                s.text = stGet(*source, "value");
                s.hasText = !s.text.empty();
                it.child(std::move(s));
            }
            const cfvariant *encs = attrOf(&item, "enclosure");
            if (encs && encs->m_type == cfvariant::Array && encs->m_array) {
                for (const auto &e : *encs->m_array) {
                    if (e.m_type != cfvariant::Struct) continue;
                    XmlEl en;
                    en.name = "enclosure";
                    std::string url = stGet(e, "url");
                    if (!url.empty()) en.attr("url", url);
                    std::string len = stGet(e, "length");
                    if (!len.empty() && len != "0") en.attr("length", len);
                    std::string typ = stGet(e, "type");
                    if (!typ.empty()) en.attr("type", typ);
                    it.child(std::move(en));
                }
            }
            const cfvariant *itemCats = attrOf(&item, "category");
            if (itemCats && itemCats->m_type == cfvariant::Array && itemCats->m_array) {
                for (const auto &cat : *itemCats->m_array) {
                    if (cat.m_type != cfvariant::Struct) continue;
                    XmlEl c;
                    c.name = "category";
                    std::string domain = stGet(cat, "domain");
                    if (!domain.empty()) c.attr("domain", domain);
                    c.text = stGet(cat, "value");
                    c.hasText = !c.text.empty();
                    it.child(std::move(c));
                }
            }
            // pubDate/expirationDate come after category and before
            // author/comments/guid (RSS093Generator.populateItem).
            std::string pub = rssDateFromValue(item, "pubDate");
            if (!pub.empty()) it.child(rssSimpleEl("pubDate", pub));
            std::string exp = rssDateFromValue(item, "expirationDate");
            if (!exp.empty()) it.child(rssSimpleEl("expirationDate", exp));
            std::string author = stGet(item, "author");
            if (!author.empty()) it.child(rssSimpleEl("author", author));
            std::string comments = stGet(item, "comments");
            if (!comments.empty()) it.child(rssSimpleEl("comments", comments));
            const cfvariant *guid = attrOf(&item, "guid");
            if (guid && guid->m_type == cfvariant::Struct) {
                XmlEl g;
                g.name = "guid";
                std::string value = stGet(*guid, "value");
                g.text = value;
                g.hasText = !value.empty();
                std::string permalink = stGet(*guid, "isPermaLink");
                if (!permalink.empty()) {
                    std::string pl = lower(permalink);
                    if (pl != "true" && pl != "yes") g.attr("isPermaLink", "false");
                }
                it.child(std::move(g));
            }
            channel.child(std::move(it));
        }
    }
    rss.child(std::move(channel));
    (void)encoding;
    return rss;
}

// ---- Atom 1.0 generation ----

XmlEl atomSimpleEl(const char *name, const std::string &value)
{
    XmlEl el;
    el.name = name;
    el.text = value;
    el.hasText = !value.empty();
    return el;
}

XmlEl atomContentEl(const char *name, const std::string &type, const std::string &value)
{
    XmlEl el;
    el.name = name;
    if (!type.empty()) el.attr("type", type);
    el.text = value;
    el.hasText = !value.empty();
    return el;
}

XmlEl atomPersonEl(const char *name, const cfvariant &person)
{
    XmlEl el;
    el.name = name;
    insertNotNull(el, "name", stGet(person, "name"));
    insertNotNull(el, "uri", stGet(person, "uri"));
    insertNotNull(el, "email", stGet(person, "email"));
    return el;
}

XmlEl atomLinkEl(const cfvariant &link)
{
    XmlEl el;
    el.name = "link";
    std::string rel = stGet(link, "rel");
    if (!rel.empty()) el.attr("rel", rel);
    std::string type = stGet(link, "type");
    if (!type.empty()) el.attr("type", type);
    std::string href = stGet(link, "href");
    if (!href.empty()) el.attr("href", href);
    std::string hreflang = stGet(link, "hreflang");
    if (!hreflang.empty()) el.attr("hreflang", hreflang);
    std::string title = stGet(link, "title");
    if (!title.empty()) el.attr("title", title);
    std::string len = stGet(link, "length");
    if (!len.empty()) el.attr("length", len);
    return el;
}

XmlEl buildAtomFeedXml(const cfvariant &feedStruct, const std::string &encoding)
{
    XmlEl feed;
    feed.name = "feed";
    feed.attr("xmlns", "http://www.w3.org/2005/Atom");
    // Feed header element order follows Rome's Atom10Generator.populateFeedHeader:
    // title, links, category, author, contributor, subtitle, id, generator,
    // rights, icon, logo, updated.
    const cfvariant *title = attrOf(&feedStruct, "title");
    if (title) {
        if (title->m_type == cfvariant::Struct) {
            feed.child(atomContentEl("title", stGet(*title, "type"), stGet(*title, "value")));
        } else {
            feed.child(atomContentEl("title", "", cfml::safe_to_std_string(*title)));
        }
    }
    const cfvariant *links = attrOf(&feedStruct, "link");
    if (links && links->m_type == cfvariant::Array && links->m_array) {
        for (const auto &l : *links->m_array) {
            if (l.m_type == cfvariant::Struct) feed.child(atomLinkEl(l));
        }
    } else if (links && links->m_type == cfvariant::String) {
        XmlEl l;
        l.name = "link";
        l.attr("href", links->m_str ? links->m_str->constData() : "");
        feed.child(std::move(l));
    }
    const cfvariant *fcats = attrOf(&feedStruct, "category");
    if (fcats && fcats->m_type == cfvariant::Array && fcats->m_array) {
        for (const auto &c : *fcats->m_array) {
            if (c.m_type != cfvariant::Struct) continue;
            XmlEl ce;
            ce.name = "category";
            std::string term = stGet(c, "term");
            if (!term.empty()) ce.attr("term", term);
            std::string label = stGet(c, "label");
            if (!label.empty()) ce.attr("label", label);
            std::string scheme = stGet(c, "scheme");
            if (!scheme.empty()) ce.attr("scheme", scheme);
            feed.child(std::move(ce));
        }
    }
    const cfvariant *author = attrOf(&feedStruct, "author");
    if (author && author->m_type == cfvariant::Array && author->m_array) {
        for (const auto &a : *author->m_array) {
            if (a.m_type == cfvariant::Struct) feed.child(atomPersonEl("author", a));
        }
    }
    const cfvariant *contributor = attrOf(&feedStruct, "contributor");
    if (contributor && contributor->m_type == cfvariant::Array && contributor->m_array) {
        for (const auto &a : *contributor->m_array) {
            if (a.m_type == cfvariant::Struct) feed.child(atomPersonEl("contributor", a));
        }
    }
    const cfvariant *subtitle = attrOf(&feedStruct, "subtitle");
    if (subtitle && subtitle->m_type == cfvariant::Struct) {
        feed.child(atomContentEl("subtitle", stGet(*subtitle, "type"), stGet(*subtitle, "value")));
    }
    insertNotNull(feed, "id", stGet(feedStruct, "id"));
    insertNotNull(feed, "rights", stGet(feedStruct, "rights"));
    std::string updated = atomDateFromValue(feedStruct, "updated");
    if (!updated.empty()) feed.child(atomSimpleEl("updated", updated));
    // entries
    const cfvariant *entries = attrOf(&feedStruct, "entry");
    if (entries && entries->m_type == cfvariant::Array && entries->m_array) {
        for (const auto &e : *entries->m_array) {
            if (e.m_type != cfvariant::Struct) continue;
            XmlEl en;
            en.name = "entry";
            // Entry element order follows Atom10Generator.populateEntry:
            // title, links, category, author, contributor, id, updated,
            // published, content, summary.
            const cfvariant *t = attrOf(&e, "title");
            if (t) {
                if (t->m_type == cfvariant::Struct) en.child(atomContentEl("title", stGet(*t, "type"), stGet(*t, "value")));
                else en.child(atomContentEl("title", "", cfml::safe_to_std_string(*t)));
            }
            const cfvariant *links2 = attrOf(&e, "link");
            if (links2 && links2->m_type == cfvariant::Array && links2->m_array) {
                for (const auto &l : *links2->m_array) {
                    if (l.m_type == cfvariant::Struct) en.child(atomLinkEl(l));
                }
            }
            const cfvariant *ecats = attrOf(&e, "category");
            if (ecats && ecats->m_type == cfvariant::Array && ecats->m_array) {
                for (const auto &c : *ecats->m_array) {
                    if (c.m_type != cfvariant::Struct) continue;
                    XmlEl ce;
                    ce.name = "category";
                    std::string term = stGet(c, "term");
                    if (!term.empty()) ce.attr("term", term);
                    std::string label = stGet(c, "label");
                    if (!label.empty()) ce.attr("label", label);
                    std::string scheme = stGet(c, "scheme");
                    if (!scheme.empty()) ce.attr("scheme", scheme);
                    en.child(std::move(ce));
                }
            }
            const cfvariant *eauthors = attrOf(&e, "author");
            if (eauthors && eauthors->m_type == cfvariant::Array && eauthors->m_array) {
                for (const auto &a : *eauthors->m_array) {
                    if (a.m_type == cfvariant::Struct) en.child(atomPersonEl("author", a));
                }
            }
            const cfvariant *econtrib = attrOf(&e, "contributor");
            if (econtrib && econtrib->m_type == cfvariant::Array && econtrib->m_array) {
                for (const auto &a : *econtrib->m_array) {
                    if (a.m_type == cfvariant::Struct) en.child(atomPersonEl("contributor", a));
                }
            }
            insertNotNull(en, "id", stGet(e, "id"));
            std::string upd = atomDateFromValue(e, "updated");
            if (!upd.empty()) en.child(atomSimpleEl("updated", upd));
            std::string published = atomDateFromValue(e, "published");
            if (!published.empty()) en.child(atomSimpleEl("published", published));
            const cfvariant *content = attrOf(&e, "content");
            if (content && content->m_type == cfvariant::Array && content->m_array && !content->m_array->empty()) {
                const cfvariant &c0 = (*content->m_array)[0];
                if (c0.m_type == cfvariant::Struct) {
                    en.child(atomContentEl("content", stGet(c0, "type"), stGet(c0, "value")));
                }
            }
            const cfvariant *summary = attrOf(&e, "summary");
            if (summary && summary->m_type == cfvariant::Struct) {
                en.child(atomContentEl("summary", stGet(*summary, "type"), stGet(*summary, "value")));
            }
            feed.child(std::move(en));
        }
    }
    (void)encoding;
    return feed;
}

// Top-level feed render: XML declaration + root element (Rome pretty format,
// CRLF line separators).
std::string renderFeedXml(const XmlEl &root, const std::string &encoding)
{
    std::string enc = encoding.empty() ? "UTF-8" : encoding;
    std::string out = "<?xml version=\"1.0\" encoding=\"" + enc + "\"?>\r\n";
    printXmlEl(root, 0, out);
    return out;
}

// The Atom entries query (FeedTable column set, all VARCHAR), one row per entry.
void atomEntriesQuery(xmlNodePtr feed, cfvariant &q)
{
    webstrada::QueryData *qd = q.m_query;
    static const char *cols[] = {
        "TITLE","TITLETYPE","PUBLISHEDDATE","EXPIRATIONDATE","COMMENTS","CREATEDDATE",
        "UPDATEDDATE","RIGHTS","SOURCE","SOURCEURL","CATEGORYSCHEME","CATEGORYLABEL",
        "CATEGORYTERM","LINKREL","LINKHREF","LINKLENGTH","LINKTYPE","LINKHREFLANG",
        "LINKTITLE","RSSLINK","AUTHORNAME","AUTHORURI","AUTHOREMAIL","CONTRIBUTORNAME",
        "CONTRIBUTORURI","CONTRIBUTOREMAIL","ID","IDPERMALINK","CONTENT","CONTENTTYPE",
        "CONTENTSRC","CONTENTMODE","URI","XMLBASE","SUMMARY","SUMMARYTYPE","SUMMARYSRC",
        "SUMMARYMODE"
    };
    for (const char *c : cols) {
        webstrada::QueryColumn col;
        col.name = c;
        col.type = "VARCHAR";
        qd->columns.push_back(std::move(col));
    }
    auto setCell = [&](int row, const char *col, const std::string &v) {
        for (size_t ci = 0; ci < qd->columns.size(); ci++) {
            if (qd->columns[ci].name.compareCaseInsensitive(col) == 0) {
                while (static_cast<int>(qd->columns[ci].values.size()) <= row) qd->columns[ci].values.emplace_back(cfvariant(cfvariant::Null));
                if (!v.empty()) qd->columns[ci].values[row] = cfvariant(v.c_str());
                return;
            }
        }
    };
    std::vector<xmlNodePtr> entries = childrenNamed(feed, "entry");
    int row = 0;
    for (xmlNodePtr entry : entries) {
        std::string xmlbase = propText(entry, "base");
        setCell(row, "XMLBASE", xmlbase);
        setCell(row, "ID", childText(entry, "id"));
        setCell(row, "PUBLISHEDDATE", childText(entry, "published"));
        setCell(row, "RIGHTS", childText(entry, "rights"));
        setCell(row, "TITLE", childText(entry, "title"));
        setCell(row, "TITLETYPE", childAttr(entry, "title", "type"));
        setCell(row, "UPDATEDDATE", childText(entry, "updated"));
        setCell(row, "CREATEDDATE", childText(entry, "created"));
        // categories
        std::vector<std::string> terms, schemes, labels;
        for (xmlNodePtr c : childrenNamed(entry, "category")) {
            terms.push_back(childAttr(entry, "category", "term").empty() ? " " : childAttr(entry, "category", "term"));
            schemes.push_back(childAttr(entry, "category", "scheme").empty() ? " " : childAttr(entry, "category", "scheme"));
            labels.push_back(childAttr(entry, "category", "label").empty() ? " " : childAttr(entry, "category", "label"));
        }
        if (!terms.empty()) {
            setCell(row, "CATEGORYTERM", listJoin(terms));
            setCell(row, "CATEGORYSCHEME", listJoin(schemes));
            setCell(row, "CATEGORYLABEL", listJoin(labels));
        }
        // authors
        std::vector<std::string> an, au, ae;
        for (xmlNodePtr a : childrenNamed(entry, "author")) {
            an.push_back(childText(a, "name").empty() ? " " : childText(a, "name"));
            au.push_back(childText(a, "uri").empty() ? " " : childText(a, "uri"));
            ae.push_back(childText(a, "email").empty() ? " " : childText(a, "email"));
        }
        if (!an.empty()) {
            setCell(row, "AUTHORNAME", listJoin(an));
            setCell(row, "AUTHORURI", listJoin(au));
            setCell(row, "AUTHOREMAIL", listJoin(ae));
        }
        // links
        std::vector<std::string> lh, lr, lt, lhl, lti, ll;
        for (xmlNodePtr l : childrenNamed(entry, "link")) {
            lh.push_back(childAttr(entry, "link", "href").empty() ? " " : childAttr(entry, "link", "href"));
            lr.push_back(childAttr(entry, "link", "rel").empty() ? " " : childAttr(entry, "link", "rel"));
            lt.push_back(childAttr(entry, "link", "type").empty() ? " " : childAttr(entry, "link", "type"));
            lhl.push_back(childAttr(entry, "link", "hreflang").empty() ? " " : childAttr(entry, "link", "hreflang"));
            lti.push_back(childAttr(entry, "link", "title").empty() ? " " : childAttr(entry, "link", "title"));
            std::string len = childAttr(entry, "link", "length");
            ll.push_back(len.empty() ? " " : len);
        }
        if (!lh.empty()) {
            setCell(row, "LINKHREF", listJoin(lh));
            setCell(row, "LINKREL", listJoin(lr));
            setCell(row, "LINKTYPE", listJoin(lt));
            setCell(row, "LINKHREFLANG", listJoin(lhl));
            setCell(row, "LINKTITLE", listJoin(lti));
            setCell(row, "LINKLENGTH", listJoin(ll));
        }
        // summary / content
        setCell(row, "SUMMARY", childText(entry, "summary"));
        setCell(row, "SUMMARYTYPE", childAttr(entry, "summary", "type"));
        setCell(row, "SUMMARYSRC", childAttr(entry, "summary", "src"));
        setCell(row, "CONTENT", childText(entry, "content"));
        setCell(row, "CONTENTTYPE", childAttr(entry, "content", "type"));
        setCell(row, "CONTENTSRC", childAttr(entry, "content", "src"));
        row++;
    }
    qd->m_rowCount = row;
}

// ---- query-based create (FeedGenerator.createFeed) ----

// Reads a FeedTable column's cell (row 0-based) from a query, mapping the
// column name through `columnMap` (FeedTable column -> query column). Returns
// "" for a missing column/row.
std::string feedQueryCell(const cfvariant &q, int row, const char *colName,
                          const cfvariant *columnMap)
{
    webstrada::QueryData *qd = q.m_query;
    if (!qd) return "";
    std::string col = colName;
    if (columnMap && columnMap->m_type == cfvariant::Struct) {
        string k(colName);
        auto it = columnMap->m_struct->find(k);
        if (it != columnMap->m_struct->end()) {
            col = cfml::safe_to_std_string(it->second);
        }
    }
    int ci = qd->findColumn(col.c_str());
    if (ci < 0 || row >= static_cast<int>(qd->columns[ci].values.size())) return "";
    std::string v = cfml::safe_to_std_string(qd->columns[ci].values[row]);
    if (v == " ") return "";
    return v;
}

// Splits a comma-delimited FeedTable cell into non-blank tokens.
std::vector<std::string> feedCellList(const std::string &cell)
{
    std::vector<std::string> out;
    std::string cur;
    for (size_t i = 0; i <= cell.size(); i++) {
        if (i == cell.size() || cell[i] == ',') {
            if (!cur.empty()) out.push_back(cur);
            cur.clear();
        } else {
            cur += cell[i];
        }
    }
    return out;
}

// Builds the RSS 2.0 channel from a query (RSSGenerator.createRSSChannel query
// path): the channel metadata comes from `props`, each row becomes an item
// mapped from the FeedTable columns.
void buildRssFeedFromQuery(XmlEl &rss, const cfvariant &props, const cfvariant &q,
                           const cfvariant *columnMap)
{
    XmlEl channel;
    channel.name = "channel";
    insertNotNull(channel, "title", stGet(props, "title"));
    insertNotNull(channel, "link", stGet(props, "link"));
    insertNotNull(channel, "description", stGet(props, "description"));
    insertNotNull(channel, "language", stGet(props, "language"));
    insertNotNull(channel, "rating", stGet(props, "rating"));
    insertNotNull(channel, "copyright", stGet(props, "copyright"));
    std::string pubDate = rssDateFromValue(props, "pubDate");
    if (!pubDate.empty()) channel.child(rssSimpleEl("pubDate", pubDate));
    std::string lastBuild = rssDateFromValue(props, "lastBuildDate");
    if (!lastBuild.empty()) channel.child(rssSimpleEl("lastBuildDate", lastBuild));
    insertNotNull(channel, "docs", stGet(props, "docs"));
    insertNotNull(channel, "managingEditor", stGet(props, "managingeditor"));
    insertNotNull(channel, "webMaster", stGet(props, "webMaster"));
    insertNotNull(channel, "generator", stGet(props, "generator"));
    std::string ttl = stGet(props, "ttl");
    if (!ttl.empty()) channel.child(rssSimpleEl("ttl", ttl));

    webstrada::QueryData *qd = q.m_query;
    int rows = qd ? qd->rowCount() : 0;
    for (int r = 0; r < rows; r++) {
        XmlEl it;
        it.name = "item";
        insertNotNull(it, "title", feedQueryCell(q, r, "TITLE", columnMap));
        std::string descType = feedQueryCell(q, r, "CONTENTTYPE", columnMap);
        std::string descValue = feedQueryCell(q, r, "CONTENT", columnMap);
        if (descType != "" || descValue != "") {
            XmlEl d;
            d.name = "description";
            d.text = descValue;
            d.hasText = !descValue.empty();
            it.child(std::move(d));
        }
        insertNotNull(it, "link", feedQueryCell(q, r, "RSSLINK", columnMap));
        insertNotNull(it, "uri", feedQueryCell(q, r, "URI", columnMap));
        std::string srcUrl = feedQueryCell(q, r, "SOURCEURL", columnMap);
        std::string srcValue = feedQueryCell(q, r, "SOURCE", columnMap);
        if (srcUrl != "" || srcValue != "") {
            XmlEl s;
            s.name = "source";
            if (!srcUrl.empty()) s.attr("url", srcUrl);
            s.text = srcValue;
            s.hasText = !srcValue.empty();
            it.child(std::move(s));
        }
        std::string permalink = feedQueryCell(q, r, "IDPERMALINK", columnMap);
        std::string idVal = feedQueryCell(q, r, "ID", columnMap);
        if (permalink != "" || idVal != "") {
            XmlEl g;
            g.name = "guid";
            g.text = idVal;
            g.hasText = !idVal.empty();
            if (!permalink.empty()) {
                std::string pl = lower(permalink);
                if (pl != "true" && pl != "yes") g.attr("isPermaLink", "false");
            }
            it.child(std::move(g));
        }
        // Enclosures: LINKHREF/LINKLENGTH/LINKTYPE are comma-aligned lists.
        std::vector<std::string> hrefs = feedCellList(feedQueryCell(q, r, "LINKHREF", columnMap));
        std::vector<std::string> lens = feedCellList(feedQueryCell(q, r, "LINKLENGTH", columnMap));
        std::vector<std::string> types = feedCellList(feedQueryCell(q, r, "LINKTYPE", columnMap));
        for (size_t i = 0; i < hrefs.size(); i++) {
            XmlEl e;
            e.name = "enclosure";
            if (!hrefs[i].empty()) e.attr("url", hrefs[i]);
            if (i < lens.size() && !lens[i].empty() && lens[i] != "0") e.attr("length", lens[i]);
            if (i < types.size() && !types[i].empty()) e.attr("type", types[i]);
            it.child(std::move(e));
        }
        // Categories: CATEGORYSCHEME / CATEGORYLABEL are comma-aligned lists.
        std::vector<std::string> schemes = feedCellList(feedQueryCell(q, r, "CATEGORYSCHEME", columnMap));
        std::vector<std::string> values = feedCellList(feedQueryCell(q, r, "CATEGORYLABEL", columnMap));
        for (size_t i = 0; i < values.size(); i++) {
            XmlEl c;
            c.name = "category";
            if (i < schemes.size() && !schemes[i].empty()) c.attr("domain", schemes[i]);
            c.text = values[i];
            c.hasText = !values[i].empty();
            it.child(std::move(c));
        }
        insertNotNull(it, "comments", feedQueryCell(q, r, "COMMENTS", columnMap));
        insertNotNull(it, "author", feedQueryCell(q, r, "AUTHOREMAIL", columnMap));
        std::string pub = feedQueryCell(q, r, "PUBLISHEDDATE", columnMap);
        if (!pub.empty()) {
            double days = 0;
            cfvariant dt(cfvariant::DateTime);
            if (feedParseDate(pub, days)) {
                dt.m_double = days;
                it.child(rssSimpleEl("pubDate", formatRFC822(dt)));
            } else {
                it.child(rssSimpleEl("pubDate", pub));
            }
        }
        std::string exp = feedQueryCell(q, r, "EXPIRATIONDATE", columnMap);
        if (!exp.empty()) {
            double days = 0;
            cfvariant dt(cfvariant::DateTime);
            if (feedParseDate(exp, days)) {
                dt.m_double = days;
                it.child(rssSimpleEl("expirationDate", formatRFC822(dt)));
            } else {
                it.child(rssSimpleEl("expirationDate", exp));
            }
        }
        channel.child(std::move(it));
    }
    rss.child(std::move(channel));
}

// Builds the Atom 1.0 feed from a query (AtomGenerator.createATOMFeed query
// path).
void buildAtomFeedFromQuery(XmlEl &feed, const cfvariant &props, const cfvariant &q,
                            const cfvariant *columnMap)
{
    feed.name = "feed";
    feed.attr("xmlns", "http://www.w3.org/2005/Atom");
    const cfvariant *title = attrOf(&props, "title");
    if (title) {
        if (title->m_type == cfvariant::Struct) {
            feed.child(atomContentEl("title", stGet(*title, "type"), stGet(*title, "value")));
        } else {
            feed.child(atomContentEl("title", "", cfml::safe_to_std_string(*title)));
        }
    }
    insertNotNull(feed, "id", stGet(props, "id"));
    insertNotNull(feed, "rights", stGet(props, "rights"));
    std::string updated = atomDateFromValue(props, "updated");
    if (!updated.empty()) feed.child(atomSimpleEl("updated", updated));

    webstrada::QueryData *qd = q.m_query;
    int rows = qd ? qd->rowCount() : 0;
    for (int r = 0; r < rows; r++) {
        XmlEl en;
        en.name = "entry";
        std::string tval = feedQueryCell(q, r, "TITLE", columnMap);
        std::string ttype = feedQueryCell(q, r, "TITLETYPE", columnMap);
        if (tval != "" || ttype != "") en.child(atomContentEl("title", ttype, tval));
        insertNotNull(en, "id", feedQueryCell(q, r, "ID", columnMap));
        std::string upd = feedQueryCell(q, r, "UPDATEDDATE", columnMap);
        if (!upd.empty()) {
            double days = 0;
            cfvariant dt(cfvariant::DateTime);
            if (feedParseDate(upd, days)) { dt.m_double = days; en.child(atomSimpleEl("updated", formatW3CDateTime(dt))); }
            else en.child(atomSimpleEl("updated", upd));
        }
        std::string published = feedQueryCell(q, r, "PUBLISHEDDATE", columnMap);
        if (!published.empty()) {
            double days = 0;
            cfvariant dt(cfvariant::DateTime);
            if (feedParseDate(published, days)) { dt.m_double = days; en.child(atomSimpleEl("published", formatW3CDateTime(dt))); }
            else en.child(atomSimpleEl("published", published));
        }
        // Author: AUTHORNAME/AUTHORURI/AUTHOREMAIL comma-aligned lists.
        std::vector<std::string> names = feedCellList(feedQueryCell(q, r, "AUTHORNAME", columnMap));
        std::vector<std::string> uris = feedCellList(feedQueryCell(q, r, "AUTHORURI", columnMap));
        std::vector<std::string> emails = feedCellList(feedQueryCell(q, r, "AUTHOREMAIL", columnMap));
        for (size_t i = 0; i < names.size(); i++) {
            XmlEl a;
            a.name = "author";
            if (i < uris.size() && !uris[i].empty()) a.child(atomSimpleEl("uri", uris[i]));
            a.child(atomSimpleEl("name", names[i]));
            if (i < emails.size() && !emails[i].empty()) a.child(atomSimpleEl("email", emails[i]));
            en.child(std::move(a));
        }
        // Links: LINKHREF/LINKREL/LINKTYPE/LINKHREFLANG/LINKTITLE/LINKLENGTH lists.
        std::vector<std::string> hrefs = feedCellList(feedQueryCell(q, r, "LINKHREF", columnMap));
        std::vector<std::string> rels = feedCellList(feedQueryCell(q, r, "LINKREL", columnMap));
        std::vector<std::string> types = feedCellList(feedQueryCell(q, r, "LINKTYPE", columnMap));
        std::vector<std::string> hreflangs = feedCellList(feedQueryCell(q, r, "LINKHREFLANG", columnMap));
        std::vector<std::string> titles = feedCellList(feedQueryCell(q, r, "LINKTITLE", columnMap));
        std::vector<std::string> lens = feedCellList(feedQueryCell(q, r, "LINKLENGTH", columnMap));
        for (size_t i = 0; i < hrefs.size(); i++) {
            XmlEl l;
            l.name = "link";
            if (i < rels.size() && !rels[i].empty()) l.attr("rel", rels[i]);
            if (i < types.size() && !types[i].empty()) l.attr("type", types[i]);
            l.attr("href", hrefs[i]);
            if (i < hreflangs.size() && !hreflangs[i].empty()) l.attr("hreflang", hreflangs[i]);
            if (i < titles.size() && !titles[i].empty()) l.attr("title", titles[i]);
            if (i < lens.size() && !lens[i].empty()) l.attr("length", lens[i]);
            en.child(std::move(l));
        }
        std::string summary = feedQueryCell(q, r, "SUMMARY", columnMap);
        std::string summaryType = feedQueryCell(q, r, "SUMMARYTYPE", columnMap);
        if (summary != "" || summaryType != "") en.child(atomContentEl("summary", summaryType, summary));
        std::string content = feedQueryCell(q, r, "CONTENT", columnMap);
        std::string contentSrc = feedQueryCell(q, r, "CONTENTSRC", columnMap);
        std::string contentType = feedQueryCell(q, r, "CONTENTTYPE", columnMap);
        if (content != "" || contentType != "" || contentSrc != "") {
            XmlEl c;
            c.name = "content";
            if (!contentType.empty()) c.attr("type", contentType);
            if (!contentSrc.empty()) c.attr("src", contentSrc);
            c.text = content;
            c.hasText = !content.empty();
            en.child(std::move(c));
        }
        feed.child(std::move(en));
    }
}

} // namespace

void cf_feed_tag(const cfvariant *attrs,
                 void *cgi, void *server, void *cookie, void *application,
                 void *session, void *url, void *form, void *variables)
{
    std::string action = attrStr(attrs, "action");
    if (action.empty()) action = "read";
    std::string actionLow = lower(action);

    std::string source = attrStr(attrs, "source");
    std::string nameAttr = attrStr(attrs, "name");
    std::string propertiesAttr = attrStr(attrs, "properties");
    std::string queryAttr = attrStr(attrs, "query");
    std::string xmlVar = attrStr(attrs, "xmlvar");
    std::string outputFile = attrStr(attrs, "outputfile");
    bool overwrite = attrBool(attrs, "overwrite", false);
    const cfvariant *columnMap = attrOf(attrs, "columnmap");

    if (actionLow == "read") {
        if (source.empty()) {
            throwApp("The source attribute is required for the read action.");
        }
        std::string userAgent = attrStr(attrs, "useragent");
        if (userAgent.empty()) userAgent = "ColdFusion";
        long timeout = 0;
        if (const cfvariant *t = attrOf(attrs, "timeout")) {
            std::string ts = cfml::safe_to_std_string(*t);
            if (!ts.empty()) {
                try { timeout = std::stol(ts); } catch (...) {}
            }
        }
        std::string xml;
        if (!readFeedSource(source, xml, userAgent, timeout)) {
            throwApp("The source " + source + " could not be read.");
        }
        xmlDocPtr doc = xmlReadMemory(xml.c_str(), static_cast<int>(xml.size()), "feed.xml", nullptr,
                                      XML_PARSE_NONET | XML_PARSE_NOCDATA);
        if (!doc) {
            throwApp("An error occurred while Parsing an XML document.");
        }
        xmlNodePtr root = xmlDocGetRootElement(doc);
        if (!root) {
            xmlFreeDoc(doc);
            throwApp("An error occurred while Parsing an XML document.");
        }

        std::string rootName = lower(reinterpret_cast<const char*>(root->name));
        cfvariant metadata(cfvariant::Struct);
        cfvariant feedStruct(cfvariant::Struct);
        cfvariant feedQuery(cfvariant::Query);
        std::string feedVersion;
        std::string feedEncoding;

        if (rootName == "rss" || rootName == "rdf") {
            xmlNodePtr channel = findChild(root, "channel");
            if (!channel && rootName == "rdf") {
                // RSS 1.0: <RDF><channel>...</channel><item>...</item></RDF>
                channel = findChild(root, "channel");
            }
            if (channel) {
                std::string verProp = propText(root, "version");
                std::string version = rootName == "rdf" ? "1.0" : (verProp.empty() ? "2.0" : verProp);
                if (version == "0.90" || version == "0.91" || version == "0.92" ||
                    version == "0.93" || version == "0.94" || version == "1.0" ||
                    version == "2.0") {
                    feedVersion = "rss_" + version;
                } else {
                    feedVersion = "rss_2.0";
                }
                if (version == "0.90") feedVersion = "rss_0.90";
                else if (version == "0.91") feedVersion = "rss_0.91";
                else if (version == "0.92") feedVersion = "rss_0.92";
                else if (version == "0.93") feedVersion = "rss_0.93";
                else if (version == "0.94") feedVersion = "rss_0.94";
                else if (version == "1.0") feedVersion = "rss_1.0";
                else feedVersion = "rss_2.0";

                double rssVersion = std::strtod(version.c_str(), nullptr);
                if (rssVersion == 0.0 && version == "0.90") rssVersion = 0.90;

                rssMetadataStruct(channel, feedVersion, feedEncoding, metadata);
                // Items: for RSS 1.0 they may live at the RDF level, for 2.0 in
                // the channel.
                std::vector<xmlNodePtr> items;
                if (rootName == "rdf") {
                    items = childrenNamed(root, "item");
                    if (items.empty()) items = childrenNamed(channel, "item");
                } else {
                    items = childrenNamed(channel, "item");
                }
                cfvariant entries(cfvariant::Array);
                if (!entries.m_array) entries.m_array = new std::vector<cfvariant>();
                for (xmlNodePtr it : items) entries.insert(rssItemStruct(it, rssVersion));
                if (!entries.m_array || !entries.m_array->empty()) metadata.set("item") = entries;
                feedStruct = metadata.deepCopy();
                // The items live at the RDF root for RSS 1.0, in the channel for
                // RSS 2.0.
                rssItemsQuery(rootName == "rdf" ? root : channel, rssVersion, feedQuery);
            }
        } else if (rootName == "feed") {
            feedVersion = "atom_1.0";
            atomMetadataStruct(root, feedVersion, feedEncoding, metadata);
            std::vector<xmlNodePtr> entries = childrenNamed(root, "entry");
            cfvariant entryArr(cfvariant::Array);
            if (!entryArr.m_array) entryArr.m_array = new std::vector<cfvariant>();
            for (xmlNodePtr e : entries) entryArr.insert(atomEntryStruct(e));
            if (!entryArr.m_array || !entryArr.m_array->empty()) metadata.set("entry") = entryArr;
            feedStruct = metadata.deepCopy();
            atomEntriesQuery(root, feedQuery);
        }
        xmlFreeDoc(doc);

        if (!propertiesAttr.empty()) {
            cfvariant_assign(static_cast<const cfvariant*>(cgi), static_cast<const cfvariant*>(server),
                             static_cast<const cfvariant*>(cookie), static_cast<const cfvariant*>(application),
                             static_cast<const cfvariant*>(session), static_cast<const cfvariant*>(url),
                             static_cast<const cfvariant*>(form), static_cast<cfvariant*>(variables),
                             propertiesAttr.c_str(), &metadata);
        }
        if (!nameAttr.empty()) {
            cfvariant_assign(static_cast<const cfvariant*>(cgi), static_cast<const cfvariant*>(server),
                             static_cast<const cfvariant*>(cookie), static_cast<const cfvariant*>(application),
                             static_cast<const cfvariant*>(session), static_cast<const cfvariant*>(url),
                             static_cast<const cfvariant*>(form), static_cast<cfvariant*>(variables),
                             nameAttr.c_str(), &feedStruct);
        }
        if (!queryAttr.empty()) {
            cfvariant_assign(static_cast<const cfvariant*>(cgi), static_cast<const cfvariant*>(server),
                             static_cast<const cfvariant*>(cookie), static_cast<const cfvariant*>(application),
                             static_cast<const cfvariant*>(session), static_cast<const cfvariant*>(url),
                             static_cast<const cfvariant*>(form), static_cast<cfvariant*>(variables),
                             queryAttr.c_str(), &feedQuery);
        }
        if (!xmlVar.empty()) {
            cfvariant x(xml.c_str());
            cfvariant_assign(static_cast<const cfvariant*>(cgi), static_cast<const cfvariant*>(server),
                             static_cast<const cfvariant*>(cookie), static_cast<const cfvariant*>(application),
                             static_cast<const cfvariant*>(session), static_cast<const cfvariant*>(url),
                             static_cast<const cfvariant*>(form), static_cast<cfvariant*>(variables),
                             xmlVar.c_str(), &x);
        }
        if (!outputFile.empty()) {
            std::error_code ec;
            if (std::filesystem::exists(outputFile) && !overwrite) {
                throwApp("The file " + outputFile + " already exists.");
            }
            std::ofstream outFile(outputFile, std::ios::binary | std::ios::trunc);
            if (!outFile.is_open()) {
                throwApp("Unable to create file " + outputFile + ".");
            }
            outFile << xml;
            outFile.close();
        }
        return;
    }

    if (actionLow == "create") {
        cfvariant feedDef(cfvariant::Struct);
        const cfvariant *nameVal = attrOf(attrs, "name");
        if (nameVal && nameVal->m_type == cfvariant::Struct) {
            feedDef = *nameVal;
        }
        const cfvariant *propsVal = attrOf(attrs, "properties");
        const cfvariant *queryVal = attrOf(attrs, "query");
        // The version comes from the `name` struct or the `properties` struct
        // (defaulting to atom_1.0 like CF's FeedGenerator.createFeed).
        std::string version = stGet(feedDef, "version");
        if (version.empty() && propsVal && propsVal->m_type == cfvariant::Struct) {
            version = stGet(*propsVal, "version");
        }
        if (version.empty()) version = "atom_1.0";
        std::string encoding = stGet(feedDef, "encoding");
        if (encoding.empty() && propsVal && propsVal->m_type == cfvariant::Struct) {
            encoding = stGet(*propsVal, "encoding");
        }

        XmlEl root;
        if (lower(version) == "rss_2.0") {
            if (feedDef.m_struct && !feedDef.m_struct->empty()) {
                root = buildRssFeedXml(feedDef, encoding);
            } else {
                // query + properties (+ columnmap) path.
                XmlEl rss;
                rss.name = "rss";
                rss.attr("version", "2.0");
                cfvariant props(cfvariant::Struct);
                if (propsVal && propsVal->m_type == cfvariant::Struct) props = *propsVal;
                if (!queryVal || queryVal->m_type != cfvariant::Query) {
                    throwApp("Exception while creating feed.",
                             "<br> query should be a CFML Query object.");
                }
                buildRssFeedFromQuery(rss, props, *queryVal, columnMap);
                root = std::move(rss);
            }
        } else {
            if (feedDef.m_struct && !feedDef.m_struct->empty()) {
                root = buildAtomFeedXml(feedDef, encoding);
            } else {
                cfvariant props(cfvariant::Struct);
                if (propsVal && propsVal->m_type == cfvariant::Struct) props = *propsVal;
                if (!queryVal || queryVal->m_type != cfvariant::Query) {
                    throwApp("Exception while creating feed.",
                             "<br> query should be a CFML Query object.");
                }
                buildAtomFeedFromQuery(root, props, *queryVal, columnMap);
            }
        }
        std::string xml = renderFeedXml(root, encoding);

        if (!xmlVar.empty()) {
            cfvariant x(xml.c_str());
            cfvariant_assign(static_cast<const cfvariant*>(cgi), static_cast<const cfvariant*>(server),
                             static_cast<const cfvariant*>(cookie), static_cast<const cfvariant*>(application),
                             static_cast<const cfvariant*>(session), static_cast<const cfvariant*>(url),
                             static_cast<const cfvariant*>(form), static_cast<cfvariant*>(variables),
                             xmlVar.c_str(), &x);
        }
        if (!outputFile.empty()) {
            std::error_code ec;
            if (std::filesystem::exists(outputFile) && !overwrite) {
                throwApp("The file " + outputFile + " already exists.");
            }
            std::ofstream outFile(outputFile, std::ios::binary | std::ios::trunc);
            if (!outFile.is_open()) {
                throwApp("Unable to create file " + outputFile + ".");
            }
            outFile << xml;
            outFile.close();
        }
        return;
    }

    throwApp("Attribute validation error for CFFEED.",
             "The value of the ACTION attribute, which is currently " + action +
             ", must be one of the values: CREATE,READ.");
}

} // namespace cfml
