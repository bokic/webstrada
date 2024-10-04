/**
 * @file tag_http.cpp
 * @brief <cfhttp> runtime (cf_http_begin / cf_http_end).
 *
 * The HTTP request context types (HttpParam / HttpRequestCtx / g_httpCtxs)
 * live in common.cpp; <cfhttpparam> is in tag_httpparam.cpp.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>
#include <webstrada/config.h>
#include <webstrada/exceptions.h>
#include <webstrada/string.h>

#include <curl/curl.h>

#include <cctype>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace {

using webstrada::cfvariant;
using webstrada::string;

static bool httpMimeIsText(const std::string &mime)
{
    std::string m = mime;
    for (auto &c : m) c = static_cast<char>(tolower(c));
    if (m == "unable to determine mime type of file.") return true;
    if (m.rfind("text", 0) == 0) return true;
    if (m.rfind("application/octet-stream", 0) == 0) return true;
    if (m.rfind("application/xml", 0) == 0) return true;
    if (m.rfind("message", 0) == 0) return true;
    if (m.size() >= 4 && m.compare(m.size() - 4, 4, "+xml") == 0) return true;
    if (m.size() >= 5 && m.compare(m.size() - 5, 5, "+json") == 0) return true;
    if (m.rfind("application/json", 0) == 0) return true;
    return false;
}

static const char *httpStatusPhrase(int code)
{
    switch (code) {
        case 100: return "Continue";
        case 101: return "Switching Protocols";
        case 102: return "Processing";
        case 200: return "OK";
        case 201: return "Created";
        case 202: return "Accepted";
        case 203: return "Non-Authoritative Information";
        case 204: return "No Content";
        case 205: return "Reset Content";
        case 206: return "Partial Content";
        case 207: return "Multi-Status";
        case 208: return "Already Reported";
        case 226: return "IM Used";
        case 300: return "Multiple Choices";
        case 301: return "Moved Permanently";
        case 302: return "Found";
        case 303: return "See Other";
        case 304: return "Not Modified";
        case 305: return "Use Proxy";
        case 307: return "Temporary Redirect";
        case 308: return "Permanent Redirect";
        case 400: return "Bad Request";
        case 401: return "Unauthorized";
        case 402: return "Payment Required";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 406: return "Not Acceptable";
        case 407: return "Proxy Authentication Required";
        case 408: return "Request Timeout";
        case 409: return "Conflict";
        case 410: return "Gone";
        case 411: return "Length Required";
        case 412: return "Precondition Failed";
        case 413: return "Request Entity Too Large";
        case 414: return "Request-URI Too Long";
        case 415: return "Unsupported Media Type";
        case 416: return "Requested Range Not Satisfiable";
        case 417: return "Expectation Failed";
        case 422: return "Unprocessable Entity";
        case 423: return "Locked";
        case 424: return "Failed Dependency";
        case 426: return "Upgrade Required";
        case 428: return "Precondition Required";
        case 429: return "Too Many Requests";
        case 431: return "Request Header Fields Too Large";
        case 500: return "Internal Server Error";
        case 501: return "Not Implemented";
        case 502: return "Bad Gateway";
        case 503: return "Service Unavailable";
        case 504: return "Gateway Timeout";
        case 505: return "HTTP Version Not Supported";
        default: return nullptr;
    }
}

static std::string httpUrlEncode(const std::string &s)
{
    static const char *hexd = "0123456789ABCDEF";
    std::string out;
    for (unsigned char c : s) {
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
            c == '-' || c == '_' || c == '.' || c == '*') {
            out.push_back(static_cast<char>(c));
        } else {
            out.push_back('%');
            out.push_back(hexd[c >> 4]);
            out.push_back(hexd[c & 0xF]);
        }
    }
    return out;
}

static std::string decodeHttpBody(const std::vector<std::byte> &buf,
                                  const std::string &respCharset,
                                  const std::string &attrCharset)
{
    std::string charset = attrCharset;
    if (charset.empty()) charset = respCharset;
    if (!charset.empty()) {
        std::string lower = charset;
        for (auto &c : lower) c = static_cast<char>(tolower(c));
        if (lower == "utf-8" || lower == "utf8") {
            std::string s(reinterpret_cast<const char*>(buf.data()), buf.size());
            return s;
        }
        if (lower == "iso-8859-1" || lower == "latin1" || lower == "latin-1" || lower == "8859_1" ||
            lower == "windows-1252" || lower == "cp1252") {
            std::string out;
            out.reserve(buf.size());
            for (auto b : buf) {
                unsigned char c = static_cast<unsigned char>(b);
                if (c < 0x80) {
                    out.push_back(static_cast<char>(c));
                } else {
                    out.push_back(static_cast<char>(0xC0 | (c >> 6)));
                    out.push_back(static_cast<char>(0x80 | (c & 0x3F)));
                }
            }
            return out;
        }
    }
    std::string s(reinterpret_cast<const char*>(buf.data()), buf.size());
    return s;
}

size_t httpBodyCb(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    std::vector<std::byte> *buf = static_cast<std::vector<std::byte>*>(userdata);
    size_t n = size * nmemb;
    buf->insert(buf->end(), reinterpret_cast<std::byte*>(ptr), reinterpret_cast<std::byte*>(ptr) + n);
    return n;
}

size_t httpHeaderCb(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    std::string *hdr = static_cast<std::string*>(userdata);
    hdr->append(ptr, size * nmemb);
    return size * nmemb;
}

} // namespace

namespace cfml {

void cf_http_begin(const cfvariant *attrs)
{
    auto *ctx = new HttpRequestCtx;
    if (attrs && attrs->m_type == cfvariant::Struct) ctx->attrs = *attrs;
    g_httpCtxs.push_back(ctx);
}

void cf_http_end(void *cgi, void *server, void *cookie, void *application,
                       void *session, void *url, void *form, void *variables)
{
    if (g_httpCtxs.empty()) {
        throw webstrada::exception("cfhttp: no active request.");
    }
    HttpRequestCtx *ctx = g_httpCtxs.back();
    g_httpCtxs.pop_back();
    std::unique_ptr<HttpRequestCtx> guard(ctx);

    auto attr = [&](const char *key) -> const cfvariant * {
        if (ctx->attrs.m_type != cfvariant::Struct || !ctx->attrs.m_struct) return nullptr;
        string k(key);
        auto it = ctx->attrs.m_struct->find(k);
        return it == ctx->attrs.m_struct->end() ? nullptr : &it->second;
    };

    std::string urlStr = attr("url") ? safe_to_std_string(*attr("url")) : "";
    std::string method = attr("method") ? safe_to_std_string(*attr("method")) : "GET";
    if (method.empty()) method = "GET";
    for (auto &c : method) c = static_cast<char>(toupper(c));
    std::string resultName = attr("result") ? safe_to_std_string(*attr("result")) : "";
    if (resultName.empty()) resultName = "cfhttp";
    for (auto &c : resultName) c = static_cast<char>(toupper(c));
    std::string getAsBinary = attr("getasbinary") ? safe_to_std_string(*attr("getasbinary")) : "";
    if (getAsBinary.empty()) getAsBinary = "NO";
    for (auto &c : getAsBinary) c = static_cast<char>(toupper(c));
    if (getAsBinary == "TRUE") getAsBinary = "YES";
    else if (getAsBinary == "FALSE") getAsBinary = "NO";
    std::string path = attr("path") ? safe_to_std_string(*attr("path")) : "";
    std::string file = attr("file") ? safe_to_std_string(*attr("file")) : "";
    std::string useragent = attr("useragent") ? safe_to_std_string(*attr("useragent")) : "ColdFusion";
    std::string username = attr("username") ? safe_to_std_string(*attr("username")) : "";
    std::string password = attr("password") ? safe_to_std_string(*attr("password")) : "";
    std::string charsetAttr = attr("charset") ? safe_to_std_string(*attr("charset")) : "";
    long long timeout = attr("timeout") ? cfvariant_to_long(attr("timeout")) : -1;
    bool redirect = !attr("redirect") || isTruthy(*attr("redirect"));
    bool throwonerror = attr("throwonerror") && isTruthy(*attr("throwonerror"));

    if (urlStr.empty()) {
        throw webstrada::exception("cfhttp: the url attribute is required.");
    }

    CURL *curl = curl_easy_init();
    if (!curl) {
        throw webstrada::exception("cfhttp: could not initialize the HTTP client.");
    }
    std::vector<std::byte> bodyBuf;
    std::string rawHeader;
    struct curl_slist *headers = nullptr;

    std::string effectiveUrl = urlStr;
    for (const auto &p : ctx->params) {
        std::string t = p.type;
        for (auto &c : t) c = static_cast<char>(tolower(c));
        if (t == "url") {
            std::string kv = p.name + "=" + (p.encoded ? httpUrlEncode(p.value) : p.value);
            if (effectiveUrl.find('?') == std::string::npos) effectiveUrl += "?";
            else if (effectiveUrl.back() != '?' && effectiveUrl.back() != '&') effectiveUrl += "&";
            effectiveUrl += kv;
        }
    }

    curl_easy_setopt(curl, CURLOPT_URL, effectiveUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, httpBodyCb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &bodyBuf);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, httpHeaderCb);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &rawHeader);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, redirect ? 1L : 0L);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, redirect ? 50L : 0L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, useragent.c_str());
    if (timeout > 0) curl_easy_setopt(curl, CURLOPT_TIMEOUT, static_cast<long>(timeout));
    if (!username.empty() || !password.empty()) {
        curl_easy_setopt(curl, CURLOPT_USERNAME, username.c_str());
        curl_easy_setopt(curl, CURLOPT_PASSWORD, password.c_str());
    }

    bool hasContentType = false;
    std::string postFields;
    bool haveBody = false;
    std::vector<std::string> bodyData;
    for (const auto &p : ctx->params) {
        std::string t = p.type;
        for (auto &c : t) c = static_cast<char>(tolower(c));
        if (t == "header" || t == "cgi") {
            if (p.name.empty()) continue;
            std::string v = p.value;
            if (t == "cgi" && p.encoded) v = httpUrlEncode(v);
            std::string hdrLine = p.name + ": " + v;
            if (p.name.compare(0, 13, "Content-Type", 0, 13) == 0 ||
                p.name.compare(0, 13, "content-type", 0, 13) == 0) hasContentType = true;
            headers = curl_slist_append(headers, hdrLine.c_str());
        } else if (t == "cookie") {
            std::string v = p.encoded ? httpUrlEncode(p.value) : p.value;
            headers = curl_slist_append(headers, ("Cookie: " + p.name + "=" + v).c_str());
        } else if (t == "formfield") {
            if (method == "GET") {
                std::string kv = p.name + "=" + (p.encoded ? httpUrlEncode(p.value) : p.value);
                if (effectiveUrl.find('?') == std::string::npos) effectiveUrl += "?";
                else if (effectiveUrl.back() != '?' && effectiveUrl.back() != '&') effectiveUrl += "&";
                effectiveUrl += kv;
            } else {
                if (!postFields.empty()) postFields += "&";
                postFields += p.name + "=" + (p.encoded ? httpUrlEncode(p.value) : p.value);
                haveBody = true;
            }
        } else if (t == "body" || t == "xml") {
            if (t == "xml") {
                if (!hasContentType) {
                    headers = curl_slist_append(headers, "Content-Type: text/xml; charset=UTF-8");
                    hasContentType = true;
                }
            }
            bodyData.push_back(p.value);
            haveBody = true;
        } else if (t == "file") {
            std::ifstream in(p.file, std::ios::binary);
            if (!in) {
                curl_slist_free_all(headers);
                curl_easy_cleanup(curl);
                throw webstrada::exception(string(("cfhttp: could not open file '" + p.file + "'.").c_str()));
            }
            std::stringstream ss;
            ss << in.rdbuf();
            bodyData.push_back(ss.str());
            haveBody = true;
        }
    }

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    std::string postData;
    if (method == "POST" || method == "PUT" || method == "PATCH" || method == "DELETE") {
        if (method == "POST") curl_easy_setopt(curl, CURLOPT_POST, 1L);
        else curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method.c_str());
        if (haveBody) {
            if (!postFields.empty()) postData = postFields;
            else {
                for (size_t i = 0; i < bodyData.size(); i++) {
                    if (i) postData += "&";
                    postData += bodyData[i];
                }
            }
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData.c_str());
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(postData.size()));
        } else {
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "");
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, 0L);
        }
    } else if (method == "HEAD") {
        curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
    } else if (method != "GET") {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method.c_str());
    }

    CURLcode rc = curl_easy_perform(curl);
    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    cfvariant result(cfvariant::Struct);
    std::string statusCode;
    std::string errorDetail;
    std::string mimetype = "Unable to determine MIME type of file.";
    std::string charset;
    if (rc != CURLE_OK) {
        statusCode = "Connection Failure.  Status code unavailable.";
        errorDetail = "Connect Exception: " + std::string(curl_easy_strerror(rc));
        if (throwonerror) {
            throw webstrada::exception("Connection Failure: Status code unavailable");
        }
    } else {
        std::string statusLine;
        size_t eol = rawHeader.find("\r\n");
        if (eol == std::string::npos) eol = rawHeader.find('\n');
        if (eol != std::string::npos) statusLine = rawHeader.substr(0, eol);
        const char *phrase = httpStatusPhrase(static_cast<int>(httpCode));
        std::string reason = phrase ? phrase : "";
        if (!reason.empty()) statusCode = std::to_string(httpCode) + " " + reason;
        else statusCode = std::to_string(httpCode);

        std::string headerBlock = rawHeader;
        if (eol != std::string::npos) headerBlock = rawHeader.substr(eol == std::string::npos ? 0 : eol + 2);
        std::string contentType;
        {
            size_t pos = 0;
            while (pos < headerBlock.size()) {
                size_t lineEnd = headerBlock.find("\r\n", pos);
                if (lineEnd == std::string::npos) lineEnd = headerBlock.find('\n', pos);
                if (lineEnd == std::string::npos) lineEnd = headerBlock.size();
                std::string line = headerBlock.substr(pos, lineEnd - pos);
                if (line.rfind("Content-Type:", 0) == 0 || line.rfind("content-type:", 0) == 0) {
                    contentType = line.substr(13);
                    while (!contentType.empty() && (contentType.front() == ' ' || contentType.front() == '\t'))
                        contentType.erase(contentType.begin());
                    break;
                }
                pos = lineEnd + (lineEnd < headerBlock.size() && headerBlock[lineEnd] == '\r' ? 2 : 1);
            }
        }
        if (!contentType.empty()) {
            std::string mimePart = contentType;
            size_t semi = mimePart.find(';');
            if (semi != std::string::npos) mimePart = mimePart.substr(0, semi);
            while (!mimePart.empty() && (mimePart.back() == ' ' || mimePart.back() == '\t')) mimePart.pop_back();
            if (!mimePart.empty()) mimetype = mimePart;
            size_t cs = contentType.find("charset=");
            if (cs != std::string::npos) {
                std::string c = contentType.substr(cs + 8);
                size_t semi2 = c.find(';');
                if (semi2 != std::string::npos) c = c.substr(0, semi2);
                while (!c.empty() && (c.front() == ' ' || c.front() == '\t' || c.front() == '"')) c.erase(c.begin());
                while (!c.empty() && (c.back() == ' ' || c.back() == '"')) c.pop_back();
                charset = c;
            }
        }
        if (throwonerror && (httpCode >= 400 || httpCode >= 300)) {
            if (httpCode >= 400) {
                std::string msg = !reason.empty() ? reason : std::to_string(httpCode);
                throw webstrada::exception(msg.c_str());
            }
        }
    }

    result.set("errorDetail") = cfvariant(errorDetail.c_str());
    result.set("mimeType") = cfvariant(mimetype.c_str());
    result.set("statusCode") = cfvariant(statusCode.c_str());

    bool isText = httpMimeIsText(mimetype);
    result.set("text") = [&] {
        cfvariant v(cfvariant::Boolean);
        v.m_bool = isText;
        return v;
    }();
    result.set("charset") = cfvariant(charset.c_str());

    cfvariant respHeaders(cfvariant::Struct);
    if (rc == CURLE_OK) {
        std::string statusLine2;
        size_t eol2 = rawHeader.find("\r\n");
        if (eol2 == std::string::npos) eol2 = rawHeader.find('\n');
        if (eol2 != std::string::npos) statusLine2 = rawHeader.substr(0, eol2);
        size_t sp1 = statusLine2.find(' ');
        std::string httpVer = sp1 == std::string::npos ? "" : statusLine2.substr(0, sp1);
        respHeaders.set("Http_Version") = cfvariant(httpVer.c_str());
        respHeaders.set("Status_Code") = cfvariant(std::to_string(httpCode).c_str());
        const char *phrase2 = httpStatusPhrase(static_cast<int>(httpCode));
        std::string reason2 = phrase2 ? phrase2 : "";
        respHeaders.set("Explanation") = cfvariant(!reason2.empty() ? reason2.c_str() : "");

        std::string block = rawHeader;
        if (eol2 != std::string::npos) block = rawHeader.substr(eol2 + 2);
        size_t pos = 0;
        while (pos < block.size()) {
            size_t lineEnd = block.find("\r\n", pos);
            if (lineEnd == std::string::npos) lineEnd = block.find('\n', pos);
            if (lineEnd == std::string::npos) lineEnd = block.size();
            std::string line = block.substr(pos, lineEnd - pos);
            size_t colon = line.find(':');
            if (colon != std::string::npos) {
                std::string name = line.substr(0, colon);
                std::string val = line.substr(colon + 1);
                while (!val.empty() && (val.front() == ' ' || val.front() == '\t')) val.erase(val.begin());
                respHeaders.set(name.c_str()) = cfvariant(val.c_str());
            }
            pos = lineEnd + (lineEnd < block.size() && block[lineEnd] == '\r' ? 2 : 1);
        }
    }
    result.set("responseHeader") = respHeaders;

    {
        std::string hdrOut;
        if (rc == CURLE_OK) {
            hdrOut = rawHeader;
            if (!hdrOut.empty() && hdrOut.back() != '\n') hdrOut += "\r\n";
        }
        result.set("header") = cfvariant(hdrOut.c_str());
    }

    if (!path.empty()) {
        std::string dir = path;
        for (auto &c : dir) if (c == '\\') c = '/';
        if (!dir.empty() && dir.back() != '/') dir += "/";
        std::string full = dir + file;
        std::ofstream outFile(full, std::ios::binary);
        if (outFile) {
            outFile.write(reinterpret_cast<const char*>(bodyBuf.data()),
                          static_cast<std::streamsize>(bodyBuf.size()));
            outFile.close();
        }
        result.set("fileContent") = cfvariant("A CFHttp.Filecontent variable is not created if a file path is specified.");
    } else if (rc != CURLE_OK) {
        result.set("fileContent") = cfvariant("Connection Failure");
    } else {
        if (getAsBinary == "YES") {
            cfvariant b(cfvariant::Binary);
            b.m_binary->assign(bodyBuf.begin(), bodyBuf.end());
            result.set("fileContent") = b;
        } else if (getAsBinary == "NEVER" || isText) {
            std::string text = decodeHttpBody(bodyBuf, charset, charsetAttr);
            result.set("fileContent") = cfvariant(text.c_str());
        } else if (getAsBinary == "AUTO") {
            cfvariant b(cfvariant::Binary);
            b.m_binary->assign(bodyBuf.begin(), bodyBuf.end());
            result.set("fileContent") = b;
        } else {
            // getasbinary="no" (the default) with a non-text MIME: CF stores the
            // raw ByteArrayOutputStream, which IsBinary reports NO for, Len
            // reports the byte count, and stringifying decodes the bytes with
            // the default charset (UTF-8, invalid -> U+FFFD). Model it as a
            // Binary marked m_isByteArrayOutputStream (was BUGS.md "cfhttp
            // getasbinary=no stores binary").
            cfvariant b(cfvariant::Binary);
            b.m_binary->assign(bodyBuf.begin(), bodyBuf.end());
            b.m_isByteArrayOutputStream = true;
            result.set("fileContent") = b;
        }
    }

    cfvariant_assign(static_cast<const cfvariant*>(cgi), static_cast<const cfvariant*>(server),
                     static_cast<const cfvariant*>(cookie), static_cast<const cfvariant*>(application),
                     static_cast<const cfvariant*>(session), static_cast<const cfvariant*>(url),
                     static_cast<const cfvariant*>(form), static_cast<cfvariant*>(variables),
                     resultName.c_str(), &result);
}

} // namespace cfml
