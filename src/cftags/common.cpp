/**
 * @file common.cpp
 * @brief Shared runtime infrastructure for the CFML tag implementations.
 *
 * Response-output state (content type/charset/headers/cookies, flushing and
 * encoding), the silent/discard buffer stack, the <cfhttp> request context,
 * the <cfloop query> scope stack, the <cftransaction> frame stack, the
 * <cfinclude> context and cfmlBoolean.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>
#include <webstrada/config.h>
#include <webstrada/exceptions.h>
#include <webstrada/string.h>

#include <sys/stat.h>
#include <unistd.h>
#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace cfml {

// <cfhttp> request context stack (tag_http.cpp / tag_httpparam.cpp). The
// HttpParam / HttpRequestCtx structs are declared in common.h.
thread_local std::vector<HttpRequestCtx*> g_httpCtxs;

// <cfloop query> scope stack (tag_loop.cpp).
thread_local std::vector<cfvariant*> g_queryScopes;

// <cfinclude> runtime context (tag_include.cpp / worker.cpp).
thread_local cfml::IncludeRuntime *g_includeRuntime = nullptr;

// <cftransaction> frame stack (tag_transaction.cpp / tag_query.cpp).
thread_local std::vector<cfml::TxFrame> g_txStack;

// <cfstoredproc> call context stack (tag_storedproc.cpp). The StoredProcCtx
// structs are declared in common.h.
thread_local std::vector<StoredProcCtx*> g_spCtxs;

// <cfinvoke> call context stack (core_component.cpp). The InvokeCtx structs are
// declared in common.h.
thread_local std::vector<InvokeCtx*> g_invokeCtxs;

void invoke_clear()
{
    for (auto *c : g_invokeCtxs) delete c;
    g_invokeCtxs.clear();
}

// <cfimport path="..."> registered component import paths (core_component.cpp).
thread_local std::vector<std::string> g_importPaths;

void import_paths_clear()
{
    g_importPaths.clear();
}

thread_local std::map<std::string, std::string> g_appMappings;

void app_mappings_clear()
{
    g_appMappings.clear();
}

void app_mappings_set(const webstrada::cfvariant *mappingsVariant)
{
    g_appMappings.clear();
    if (!mappingsVariant || mappingsVariant->m_type != webstrada::cfvariant::Struct || !mappingsVariant->m_struct) {
        return;
    }
    for (const auto &pair : *mappingsVariant->m_struct) {
        const char *kData = pair.first.constData();
        std::string key = kData ? kData : "";
        // Normalize virtual path: lowercase, ensure leading slash, strip trailing slash.
        for (auto &c : key) {
            if (c == '\\') c = '/';
            c = (char)tolower((unsigned char)c);
        }
        if (key.empty() || key[0] != '/') {
            key = "/" + key;
        }
        while (key.size() > 1 && key.back() == '/') {
            key.pop_back();
        }

        webstrada::string targetStr = const_cast<webstrada::cfvariant&>(pair.second).toString();
        const char *tData = targetStr.constData();
        std::string target = tData ? tData : "";
        for (auto &c : target) {
            if (c == '\\') c = '/';
        }
        while (target.size() > 1 && target.back() == '/') {
            target.pop_back();
        }

        g_appMappings[key] = target;
    }
}

bool app_mappings_resolve(const std::string &path, std::string &resolved)
{
    if (g_appMappings.empty()) return false;
    std::string p = path;
    for (auto &c : p) {
        if (c == '\\') c = '/';
    }
    if (p.empty() || p[0] != '/') {
        p = "/" + p;
    }
    std::string pLower = p;
    for (auto &c : pLower) {
        c = (char)tolower((unsigned char)c);
    }

    // Longest prefix match
    std::string bestKey;
    for (const auto &kv : g_appMappings) {
        const std::string &k = kv.first;
        if (pLower == k || (pLower.size() > k.size() && pLower.compare(0, k.size(), k) == 0 && pLower[k.size()] == '/')) {
            if (k.size() > bestKey.size()) {
                bestKey = k;
            }
        }
    }

    if (!bestKey.empty()) {
        const std::string &target = g_appMappings[bestKey];
        std::string rest = p.substr(bestKey.size());
        if (!rest.empty() && rest[0] == '/') {
            resolved = target + rest;
        } else if (!rest.empty()) {
            resolved = target + "/" + rest;
        } else {
            resolved = target;
        }
        resolved = std::filesystem::path(resolved).lexically_normal().string();
        return true;
    }
    return false;
}

namespace {

using webstrada::string;

static thread_local std::deque<string> g_silentBufs;
static thread_local std::deque<string*> g_silentRealBufs;









static cfml::response_write_fn s_response_write = nullptr;








static thread_local cfml::response_state g_response_state;










} // namespace

void sendHeader(const cfml::response_state &r)
{
    if (!s_response_write) return;
    string header;
    if (r.statusCode != 200) {
        header += "Status: ";
        header += string::number(r.statusCode);
        header += "\r\n";
    }
    for (const auto &h : r.headers) {
        header += h.first.c_str();
        header += ": ";
        header += h.second.c_str();
        header += "\r\n";
    }
    header += "Content-Type: ";
    header += r.contentType;
    header += ";charset=";
    header += r.charset;
    header += "\r\n";
    for (const auto &c : r.cookies) {
        header += "Set-Cookie: ";
        header += c.c_str();
        header += "\r\n";
    }
    if (!r.contentLanguage.isEmpty()) {
        header += "Content-Language: ";
        header += r.contentLanguage;
        header += "\r\n";
    }
    header += "\r\n";
    s_response_write(header.constData(), header.length());
}

thread_local string g_cli_flushed;

webstrada::string responseCharsetCanonical(const webstrada::string &charset)
{
    webstrada::string enc = charset;
    if (enc.isEmpty()) return "UTF-8";
    enc.toUpper();
    webstrada::string n;
    for (int i = 0; i < enc.length(); i++) {
        char c = enc.at(i);
        n.append(c == '_' ? '-' : c);
    }
    if (n.equals("UTF8")) return "UTF-8";
    if (n.equals("LATIN1") || n.equals("LATIN-1") || n.equals("ISO8859-1") || n.equals("8859-1")) return "ISO-8859-1";
    if (n.equals("UNICODE")) return "UTF-16";
    if (n.equals("ASCII") || n.equals("ISO646-US")) return "US-ASCII";
    if (n.equals("UTF-8") || n.equals("ISO-8859-1") || n.equals("US-ASCII") ||
        n.equals("UTF-16") || n.equals("UTF-16BE") || n.equals("UTF-16LE")) {
        return n;
    }
    throw webstrada::exception("Unsupported encoding format " + charset + ".");
}

bool isUtf8Charset(const webstrada::string &charset)
{
    webstrada::string c = charset;
    c.toUpper();
    return c.equals("UTF-8") || c.equals("UTF8");
}

std::vector<char> encodeBuffer(const string &out, const cfml::response_state &r)
{
    std::vector<char> bytes;
    string enc = responseCharsetCanonical(r.charset);
    if (r.binary || isUtf8Charset(enc)) {
        bytes.assign(out.constData(), out.constData() + out.length());
        return bytes;
    }
    cfml::stringToBytes(out, enc, bytes);
    return bytes;
}

// Reusable conversion buffer for the send paths below (avoids a per-flush heap
// allocation when the response charset is not UTF-8).
static thread_local std::vector<char> g_encodeScratch;

// Write `out` (internal UTF-8) to the web engine in the response charset.
// Binary/UTF-8 writes straight from the buffer (no copy); other charsets are
// converted into the reusable scratch buffer first. Mirrors encodeBuffer but
// for the streaming send path.
void sendEncoded(const string &out, const cfml::response_state &r)
{
    if (!s_response_write || out.isEmpty()) return;
    string enc = responseCharsetCanonical(r.charset);
    if (r.binary || isUtf8Charset(enc)) {
        s_response_write(out.constData(), out.length());
        return;
    }
    g_encodeScratch.clear();
    cfml::stringToBytes(out, enc, g_encodeScratch);
    if (!g_encodeScratch.empty()) {
        s_response_write(g_encodeScratch.data(), g_encodeScratch.size());
    }
}

void parseContentType(const string &type, string &mime, string &charset)
{
    int semi = type.indexOf(';');
    mime = (semi >= 0) ? type.left(semi).trimmed() : type.trimmed();
    if (semi < 0) return;
    string rest = type.mid(semi + 1, type.length() - semi - 1);
    string lower = rest;
    lower.toLower();
    int csPos = lower.indexOf("charset");
    if (csPos < 0) return;
    size_t p = (size_t)csPos + 7;
    while (p < (size_t)rest.length() && (rest.at((int)p) == ' ' || rest.at((int)p) == '\t')) p++;
    if (p >= (size_t)rest.length() || rest.at((int)p) != '=') return;
    p++;
    while (p < (size_t)rest.length() && (rest.at((int)p) == ' ' || rest.at((int)p) == '\t')) p++;
    string cs = rest.mid((int)p, rest.length() - (int)p).trimmed();
    int end = cs.indexOf(';');
    if (end >= 0) cs = cs.left(end).trimmed();
    if (cs.length() >= 2 &&
        ((cs.at(0) == '"' && cs.at(cs.length() - 1) == '"') ||
         (cs.at(0) == '\'' && cs.at(cs.length() - 1) == '\''))) {
        cs = cs.mid(1, cs.length() - 2).trimmed();
    }
    charset = cs;
}

void readFileBytes(const string &path, std::vector<std::byte> &out)
{
    FILE *f = fopen(path.constData(), "rb");
    if (!f) {
        throw webstrada::exception("Could not find or read the file: " + path);
    }
    char buf[65536];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0) {
        for (size_t i = 0; i < n; i++) out.push_back(std::byte(static_cast<unsigned char>(buf[i])));
    }
    fclose(f);
}


string stripCRLF(const string &s)
{
    string r;
    for (int i = 0; i < s.length(); i++) {
        char c = s.at(i);
        if (c != '\r' && c != '\n') r.append(c);
    }
    return r;
}



string encodeControlChars(const string &s)
{
    static const char *hex[] = {
        "%00", "%01", "%02", "%03", "%04", "%05", "%06", "%07", "%08", "%09",
        "%0a", "%0b", "%0c", "%0d", "%0e", "%0f", "%10", "%11", "%12", "%13",
        "%14", "%15", "%16", "%17", "%18", "%19", "%1a", "%1b", "%1c", "%1d",
        "%1e", "%1f"
    };
    string r;
    for (int i = 0; i < s.length(); i++) {
        unsigned char c = static_cast<unsigned char>(s.at(i));
        if (c < 32) {
            r.append(hex[c]);
        } else {
            r.append(static_cast<char>(c));
        }
    }
    return r;
}



bool urlContainsCFTokens(const string &url)
{
    int len = url.length();
    for (int i = 0; i + 1 < len; i++) {
        char c = url.at(i);
        if (c == '?' || c == '&') {
            if (i + 5 < len && url.mid(i + 1, 4).compareCaseInsensitive("cfid") == 0 &&
                url.at(i + 5) == '=') {
                return true;
            }
            if (i + 8 < len && url.mid(i + 1, 7).compareCaseInsensitive("cftoken") == 0 &&
                url.at(i + 8) == '=') {
                return true;
            }
        }
    }
    return false;
}


void responseSetHeader(cfml::response_state &r, const char *name,
                              const string &value)
{
    string lower(name);
    lower.toLower();
    for (auto &h : r.headers) {
        string existing(h.first.c_str());
        existing.toLower();
        if (existing.equals(lower.constData())) {
            h.second = std::string(value.constData(), value.length());
            return;
        }
    }
    r.headers.emplace_back(std::string(name), std::string(value.constData(), value.length()));
}


bool cfmlBoolean(const cfvariant *v, bool defaultValue)
{
    if (!v || v->m_type == cfvariant::Null) return defaultValue;
    string s = variantToString(*v).trimmed();
    s.toLower();
    if (s.equals("yes") || s.equals("true") || s.equals("1") || s.equals("on")) return true;
    if (s.equals("no") || s.equals("false") || s.equals("0") || s.equals("off")) return false;
    return cfvariant_is_truthy(v) != 0;
}


string *silent_buf_push()
{
    g_silentBufs.emplace_back();
    return &g_silentBufs.back();
}

void silent_buf_pop()
{
    if (!g_silentBufs.empty()) {
        g_silentBufs.pop_back();
    }
    if (!g_silentRealBufs.empty() && g_silentRealBufs.size() > g_silentBufs.size()) {
        g_silentRealBufs.pop_back();
    }
}

void silent_buf_clear()
{
    g_silentBufs.clear();
    g_silentRealBufs.clear();
}

bool silent_buf_contains(const string *buf)
{
    for (const auto &b : g_silentBufs) {
        if (&b == buf) return true;
    }
    return false;
}

string *silent_real_out()
{
    return g_silentRealBufs.empty() ? nullptr : g_silentRealBufs.back();
}

void silent_set_real_out(string *realOut)
{
    while (g_silentRealBufs.size() < g_silentBufs.size()) g_silentRealBufs.push_back(nullptr);
    if (!g_silentRealBufs.empty()) g_silentRealBufs.back() = realOut;
}


response_state &response()
{
    return g_response_state;
}

void response_begin()
{
    auto &r = response();
    r.contentType = "text/html";
    r.charset = webstrada::config::defaultOutputCharset.c_str();
    r.committed = false;
    r.binary = false;
    r.stream = nullptr;
    r.contentLanguage.clear();
    r.cookies.clear();
    r.statusCode = 200;
    r.headers.clear();
    r.headContent.clear();
    g_cli_flushed.clear();

    // Drop any silent-buffer state left over from the previous request. If a
    // <cfsilent>/<cffunction output="false"> block unwound past cf_silent_end
    // (an exception), the recorded real-out pointer in g_silentRealBufs would
    // dangle once the previous request's output buffer is gone; the next
    // <cfcontent reset> (response_apply_cfcontent -> silent_real_out) would
    // then clear() a stale buffer (ASAN stack-buffer-overflow). scope_begin
    // also clears it, but the direct-C++ test path only calls response_begin.
    silent_buf_clear();
}

void response_set_stream(void *stream)
{
    response().stream = stream;
}

void response_set_write_fn(response_write_fn fn)
{
    s_response_write = fn;
}

int response_committed()
{
    return response().committed ? 1 : 0;
}

const char *response_charset_name()
{
    return response().charset.constData();
}

const char *response_content_type_name()
{
    return response().contentType.constData();
}

int response_is_binary()
{
    return response().binary ? 1 : 0;
}


void response_send_remaining(string *out)
{
    auto &r = response();
    if (!r.stream) return;
    if (!r.committed) {
        sendHeader(r);
        r.committed = true;
    }
    // Head content (cfhtmlhead / ajaxOnLoad) precedes the body output like CF's
    // response <head>.
    if (!r.headContent.isEmpty()) {
        sendEncoded(r.headContent, r);
        r.headContent.clear();
    }
    if (out && !out->isEmpty()) {
        sendEncoded(*out, r);
    }
}

void response_add_cookie(const char *name, const char *value, const char *path)
{
    auto &r = response();
    if (r.committed) return;
    string body = name;
    body += "=";
    body += value;
    body += "; Path=";
    body += path;
    r.cookies.push_back(std::string(body.constData(), body.length()));
}

// ---- <cfsetting enablecfoutputonly> output mode ----
static thread_local bool g_cfoutputOnly = false;

bool cf_cfoutputonly_enabled()
{
    return g_cfoutputOnly;
}

void cf_cfoutputonly_set(bool enabled)
{
    g_cfoutputOnly = enabled;
}

void cf_write_output_gated(string &out, const char *text, size_t size)
{
    if (g_cfoutputOnly) return;
    if (size > 0) out.append(text, size);
}

void cf_whitespace_space_gated(string &out)
{
    if (g_cfoutputOnly) return;
    cf_whitespace_space(out);
}

std::vector<char> response_encode(const string &out)
{
    return encodeBuffer(out, response());
}

std::vector<char> response_encode_all(const string &out)
{
    auto &r = response();
    string combined;
    combined.append(g_cli_flushed);
    // Head content (ajaxOnLoad inline <script>) precedes the body output like
    // CF's response <head>.
    combined.append(r.headContent);
    combined.append(out);
    return encodeBuffer(combined, r);
}


void include_begin(IncludeRuntime *rt)
{
    g_includeRuntime = rt;
}

void include_end()
{
    g_includeRuntime = nullptr;
}

IncludeRuntime *include_context()
{
    return g_includeRuntime;
}


static cfvariant *resolveQueryScopeMember(cfvariant *query, const std::vector<string> &parts)
{
    if (!query || query->m_type != cfvariant::Query || !query->m_query) return nullptr;
    QueryData *qd = query->m_query;

    string first = parts[0];
    string firstUpper = first;
    firstUpper.toUpper();

    if (firstUpper.equals("CURRENTROW")) {
        auto *ret = new cfvariant(qd->currentRow);
        cf_register_temp(ret);
        return ret;
    }
    if (firstUpper.equals("RECORDCOUNT")) {
        auto *ret = new cfvariant(qd->rowCount());
        cf_register_temp(ret);
        return ret;
    }
    if (firstUpper.equals("COLUMNLIST")) {
        std::vector<string> names;
        for (auto &col : qd->columns) {
            string n = col.name;
            n.toUpper();
            names.push_back(n);
        }
        std::sort(names.begin(), names.end(), [](const string &a, const string &b) {
            return a.compareCaseInsensitive(b) < 0;
        });
        string list;
        for (size_t i = 0; i < names.size(); i++) {
            if (i > 0) list += ",";
            list += names[i];
        }
        auto *ret = new cfvariant(list);
        cf_register_temp(ret);
        return ret;
    }

    int colIdx = qd->findColumn(first);
    if (colIdx < 0) return nullptr;

    // The current row's cell for the column.
    cfvariant cell(cfvariant::Null);
    if (colIdx < (int)qd->columns.size() && !qd->columns[colIdx].values.empty()) {
        int row = qd->currentRow;
        if (row < 1) row = 1;
        if (row <= (int)qd->columns[colIdx].values.size()) {
            cell = qd->columns[colIdx].values[row - 1];
        }
    }
    if (parts.size() == 1) {
        auto *ret = new cfvariant(cell);
        cf_register_temp(ret);
        return ret;
    }
    cfvariant *cur = &cell;
    for (size_t i = 1; i < parts.size(); i++) {
        if (!cur || (cur->m_type != cfvariant::Struct && cur->m_type != cfvariant::Xml)) return nullptr;
        auto it = cur->m_struct->find(parts[i]);
        if (it == cur->m_struct->end()) return nullptr;
        cur = &it->second;
    }
    auto *ret = new cfvariant(*cur);
    cf_register_temp(ret);
    return ret;
}


void query_scope_clear()
{
    for (cfvariant *q : g_queryScopes) delete q;
    g_queryScopes.clear();
}


cfvariant *query_scope_resolve_member(const std::vector<string> &parts)
{
    if (!g_queryScopes.empty()) {
        for (auto it = g_queryScopes.rbegin(); it != g_queryScopes.rend(); ++it) {
            if (auto *r = resolveQueryScopeMember(*it, parts)) return r;
        }
    }
    return nullptr;
}


TxFrame *transaction_get_active(const std::string &dsn)
{
    if (!g_txStack.empty()) {
        TxFrame &top = g_txStack.back();
        if (top.dsn.empty()) {
            top.dsn = dsn;
        }
        if (top.dsn == dsn) {
            return &top;
        }
    }
    return nullptr;
}


void transaction_clear_all()
{
    while (!g_txStack.empty()) {
        TxFrame &f = g_txStack.back();
        if (f.conn) {
            if (f.inTransaction) f.conn->rollback();
            delete f.conn;
        }
        g_txStack.pop_back();
    }
}

// Drops any stored-proc call contexts left open by an exception that unwound
// past cf_storedproc_end (called from scope_begin per request).
void stored_proc_clear()
{
    for (StoredProcCtx *ctx : g_spCtxs) delete ctx;
    g_spCtxs.clear();
}

void init_server_scope(webstrada::cfvariant &serverScope)
{
    serverScope.setUpcase(false);
    serverScope.setAutoCreate();
    serverScope["coldfusion"]["appserver"] = "webstrada";
    serverScope["coldfusion"]["productname"] = "ColdFusion Server";
    serverScope["coldfusion"]["productversion"] = "2025,0,12,331922";
    serverScope["coldfusion"]["productlevel"] = "Developer";
    serverScope["coldfusion"]["updatelevel"] = "12";
    serverScope["coldfusion"]["installkit"] = "Native UNIX";
    serverScope["coldfusion"]["rootdir"] = "/opt/coldfusion/cfusion";
    serverScope["coldfusion"]["supportedlocales"] = ",Chinese (China),Chinese (Hong Kong),Chinese (Taiwan),Dutch (Belgian),Dutch (Standard),English (Australian),English (Canadian),English (New Zealand),English (UK),English (US),French (Belgian),French (Canadian),French (Standard),French (Swiss),German (Austrian),German (Standard),German (Swiss),Italian (Standard),Italian (Swiss),Japanese,Korean,Norwegian (Bokmal),Norwegian (Nynorsk),Portuguese (Brazilian),Portuguese (Standard),Spanish (Modern),Spanish (Standard),Swedish,ar,ar_AE,ar_BH,ar_DZ,ar_EG,ar_IQ,ar_JO,ar_KW,ar_LB,ar_LY,ar_MA,ar_OM,ar_QA,ar_SA,ar_SD,ar_SY,ar_TN,ar_YE,be,be_BY,bg,bg_BG,ca,ca_ES,cs,cs_CZ,da,da_DK,de,de_AT,de_CH,de_DE,de_LU,el,el_CY,el_GR,en,en_AU,en_CA,en_GB,en_IE,en_IN,en_MT,en_NZ,en_PH,en_SG,en_US,en_ZA,es,es_AR,es_BO,es_CL,es_CO,es_CR,es_CU,es_DO,es_EC,es_ES,es_GT,es_HN,es_MX,es_NI,es_PA,es_PE,es_PR,es_PY,es_SV,es_US,es_UY,es_VE,et,et_EE,fi,fi_FI,fr,fr_BE,fr_CA,fr_CH,fr_FR,fr_LU,ga,ga_IE,he,he_IL,hi,hi_IN,hr,hr_HR,hu,hu_HU,id,id_ID,is,is_IS,it,it_CH,it_IT,ja,ja_JP,ja_JP_JP_#u-ca-japanese,ko,ko_KR,lt,lt_LT,lv,lv_LV,mk,mk_MK,ms,ms_MY,mt,mt_MT,nb,nb_NO,nl,nl_BE,nl_NL,nn_NO,no,no_NO,no_NO_NY,pl,pl_PL,pt,pt_BR,pt_PT,ro,ro_RO,ru,ru_RU,sk,sk_SK,sl,sl_SI,sq,sq_AL,sr,sr_BA,sr_BA_#Latn,sr_CS,sr_ME,sr_ME_#Latn,sr_RS,sr_RS_#Latn,sr__#Latn,sv,sv_SE,th,th_TH,th_TH_TH_#u-nu-thai,tr,tr_TR,uk,uk_UA,vi,vi_VN,zh,zh_CN,zh_CN_#Hans,zh_HK,zh_HK_#Hant,zh_SG,zh_SG_#Hans,zh_TW,zh_TW_#Hant";

    serverScope["os"]["additionalinformation"] = cfml::readfile("/proc/sys/kernel/ostype");
    serverScope["os"]["arch"] = cfml::readfile("/proc/sys/kernel/arch");
    serverScope["os"]["buildnumber"] = cfml::readfile("/proc/sys/kernel/version");
    serverScope["os"]["name"] = "LINUX";
    serverScope["os"]["version"] = cfml::readfile("/proc/sys/kernel/osrelease");

    char **s = ::environ;
    while(s && *s) {
        const char *separator = strstr(*s, "=");
        if (separator) {
            webstrada::string key(*s, separator - *s);
            const char *value = separator + 1;
            serverScope["system"]["environment"][key.constData()] = value;
        }
        s++;
    }
    serverScope.setReadOnly();
}

} // namespace cfml
