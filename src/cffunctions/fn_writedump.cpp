/**
 * @file fn_writedump.cpp
 * @brief CFML writedump() built-in.
 */

#include "common.h"

#include "../cftags/common.h"
#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>
#include <webstrada/exceptions.h>
#include <webstrada/string.h>
#include <algorithm>
#include <cmath>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <set>
#include <string>
#include <vector>

using webstrada::cfvariant;
using webstrada::string;
using webstrada::UdfParamInfo;

namespace {

static thread_local std::set<const void*> g_cfdumpVisited;

} // namespace

namespace cfml {

static const std::string &cfdumpStyle()
{
    if (g_cfdump_style_cache.empty()) {
        // Raw copy of the <style> block CF 2021 emits before the first HTML
        // dump (captured from a live CF 2021 server). Newlines are converted
        // to CRLF below to match CF's output byte-for-byte.
        static const std::string style = R"CFDUMP(<style>


	table.cfdump_wddx,
	table.cfdump_xml,
	table.cfdump_struct,
	table.cfdump_varundefined,
	table.cfdump_array,
	table.cfdump_set,
	table.cfdump_query,
	table.cfdump_cfc,
	table.cfdump_object,
	table.cfdump_binary,
	table.cfdump_udf,
	table.cfdump_udfbody,
	table.cfdump_varnull,
	table.cfdump_udfarguments {
		font-size: xx-small;
		font-family: verdana, arial, helvetica, sans-serif;
	}

	table.cfdump_wddx th,
	table.cfdump_xml th,
	table.cfdump_struct th,
	table.cfdump_varundefined th,
	table.cfdump_array th,
	table.cfdump_set th,
	table.cfdump_query th,
	table.cfdump_cfc th,
	table.cfdump_object th,
	table.cfdump_binary th,
	table.cfdump_udf th,
	table.cfdump_udfbody th,
	table.cfdump_varnull th,
	table.cfdump_udfarguments th {
		text-align: left;
		color: white;
		padding: 5px;
	}

	table.cfdump_wddx td,
	table.cfdump_xml td,
	table.cfdump_struct td,
	table.cfdump_varundefined td,
	table.cfdump_array td,
	table.cfdump_set td,
	table.cfdump_query td,
	table.cfdump_cfc td,
	table.cfdump_object td,
	table.cfdump_binary td,
	table.cfdump_udf td,
	table.cfdump_udfbody td,
	table.cfdump_varnull td,
	table.cfdump_udfarguments td {
		padding: 3px;
		background-color: #ffffff;
		vertical-align : top;
	}

	table.cfdump_wddx {
		background-color: #000000;
	}
	table.cfdump_wddx th.wddx {
		background-color: #444444;
	}


	table.cfdump_xml {
		background-color: #888888;
	}
	table.cfdump_xml th.xml {
		background-color: #aaaaaa;
	}
	table.cfdump_xml td.xml {
		background-color: #dddddd;
	}

	table.cfdump_struct {
		background-color: #0000cc ;
	}
	table.cfdump_struct th.struct {
		background-color: #4444cc ;
	}
	table.cfdump_struct td.struct {
		background-color: #ccddff;
	}

	table.cfdump_varundefined {
		background-color: #CC3300 ;
	}
	table.cfdump_varundefined th.varundefined {
		background-color: #CC3300 ;
	}
	table.cfdump_varundefined td.varundefined {
		background-color: #ccddff;
	}

	table.cfdump_array {
		background-color: #006600 ;
	}
	table.cfdump_array th.array {
		background-color: #009900 ;
	}
	table.cfdump_array td.array {
		background-color: #ccffcc ;
	}

	table.cfdump_set {
		background-color: #cc6600 ;
	}
	table.cfdump_set th.set {
		background-color: #ff8c1a ;
	}
	table.cfdump_set td.set {
		background-color: #ffe1c4 ;
	}

	table.cfdump_query {
		background-color: #884488 ;
	}
	table.cfdump_query th.query {
		background-color: #aa66aa ;
	}
	table.cfdump_query td.query {
		background-color: #ffddff ;
	}


	table.cfdump_cfc {
		background-color: #ff0000;
	}
	table.cfdump_cfc th.cfc{
		background-color: #ff4444;
	}
	table.cfdump_cfc td.cfc {
		background-color: #ffcccc;
	}


	table.cfdump_object {
		background-color : #ff0000;
	}
	table.cfdump_object th.object{
		background-color: #ff4444;
	}

	table.cfdump_binary {
		background-color : #eebb00;
	}
	table.cfdump_binary th.binary {
		background-color: #ffcc44;
	}
	table.cfdump_binary td {
		font-size: x-small;
	}
	table.cfdump_udf {
		background-color: #aa4400;
	}
	table.cfdump_udf th.udf {
		background-color: #cc6600;
	}
	table.cfdump_udfarguments {
		background-color: #dddddd;
	}
	table.cfdump_udfarguments th {
		background-color: #eeeeee;
		color: #000000;
	}

</style>)CFDUMP";
        g_cfdump_style_cache.reserve(style.size() + 64);
        for (size_t i = 0; i < style.size(); i++) {
            char c = style[i];
            if (c == '\n') {
                g_cfdump_style_cache += "\r\n";
            } else {
                g_cfdump_style_cache += c;
            }
        }
    }
    return g_cfdump_style_cache;
}

static const char *html4NamedEntity(int cp)
{
    switch (cp) {
    case 160: return "&nbsp;"; case 161: return "&iexcl;"; case 162: return "&cent;"; case 163: return "&pound;";
    case 164: return "&curren;"; case 165: return "&yen;"; case 166: return "&brvbar;"; case 167: return "&sect;";
    case 168: return "&uml;"; case 169: return "&copy;"; case 170: return "&ordf;"; case 171: return "&laquo;";
    case 172: return "&not;"; case 173: return "&shy;"; case 174: return "&reg;"; case 175: return "&macr;";
    case 176: return "&deg;"; case 177: return "&plusmn;"; case 178: return "&sup2;"; case 179: return "&sup3;";
    case 180: return "&acute;"; case 181: return "&micro;"; case 182: return "&para;"; case 183: return "&middot;";
    case 184: return "&cedil;"; case 185: return "&sup1;"; case 186: return "&ordm;"; case 187: return "&raquo;";
    case 188: return "&frac14;"; case 189: return "&frac12;"; case 190: return "&frac34;"; case 191: return "&iquest;";
    case 192: return "&Agrave;"; case 193: return "&Aacute;"; case 194: return "&Acirc;"; case 195: return "&Atilde;";
    case 196: return "&Auml;"; case 197: return "&Aring;"; case 198: return "&AElig;"; case 199: return "&Ccedil;";
    case 200: return "&Egrave;"; case 201: return "&Eacute;"; case 202: return "&Ecirc;"; case 203: return "&Euml;";
    case 204: return "&Igrave;"; case 205: return "&Iacute;"; case 206: return "&Icirc;"; case 207: return "&Iuml;";
    case 208: return "&ETH;"; case 209: return "&Ntilde;"; case 210: return "&Ograve;"; case 211: return "&Oacute;";
    case 212: return "&Ocirc;"; case 213: return "&Otilde;"; case 214: return "&Ouml;"; case 215: return "&times;";
    case 216: return "&Oslash;"; case 217: return "&Ugrave;"; case 218: return "&Uacute;"; case 219: return "&Ucirc;";
    case 220: return "&Uuml;"; case 221: return "&Yacute;"; case 222: return "&THORN;"; case 223: return "&szlig;";
    case 224: return "&agrave;"; case 225: return "&aacute;"; case 226: return "&acirc;"; case 227: return "&atilde;";
    case 228: return "&auml;"; case 229: return "&aring;"; case 230: return "&aelig;"; case 231: return "&ccedil;";
    case 232: return "&egrave;"; case 233: return "&eacute;"; case 234: return "&ecirc;"; case 235: return "&euml;";
    case 236: return "&igrave;"; case 237: return "&iacute;"; case 238: return "&icirc;"; case 239: return "&iuml;";
    case 240: return "&eth;"; case 241: return "&ntilde;"; case 242: return "&ograve;"; case 243: return "&oacute;";
    case 244: return "&ocirc;"; case 245: return "&otilde;"; case 246: return "&ouml;"; case 247: return "&divide;";
    case 248: return "&oslash;"; case 249: return "&ugrave;"; case 250: return "&uacute;"; case 251: return "&ucirc;";
    case 252: return "&uuml;"; case 253: return "&yacute;"; case 254: return "&thorn;"; case 255: return "&yuml;";
    default: break;
    }
    // Entities beyond Latin-1 verified against CF 2021.
    static const struct { int cp; const char *name; } extra[] = {
        { 338, "&OElig;" }, { 710, "&circ;" }, { 732, "&tilde;" }, { 913, "&Alpha;" },
        { 945, "&alpha;" }, { 8211, "&ndash;" }, { 8217, "&rsquo;" }, { 8220, "&ldquo;" },
        { 8226, "&bull;" }, { 8364, "&euro;" }, { 8465, "&image;" }, { 8482, "&trade;" },
        { 8592, "&larr;" }, { 8721, "&sum;" }, { 9674, "&loz;" }, { 9824, "&spades;" },
    };
    for (const auto &e : extra) {
        if (e.cp == cp) return e.name;
    }
    return nullptr;
}

static int cfdumpDecodeChar(const char *data, size_t len, size_t i, size_t &consumed)
{
    unsigned char b0 = static_cast<unsigned char>(data[i]);
    if (b0 < 0x80) { consumed = 1; return b0; }
    if (b0 >= 0xC2 && b0 <= 0xDF && i + 1 < len) {
        unsigned char b1 = static_cast<unsigned char>(data[i + 1]);
        if ((b1 & 0xC0) == 0x80) {
            consumed = 2;
            return ((b0 & 0x1F) << 6) | (b1 & 0x3F);
        }
    } else if (b0 >= 0xE0 && b0 <= 0xEF && i + 2 < len) {
        unsigned char b1 = static_cast<unsigned char>(data[i + 1]);
        unsigned char b2 = static_cast<unsigned char>(data[i + 2]);
        if ((b1 & 0xC0) == 0x80 && (b2 & 0xC0) == 0x80) {
            consumed = 3;
            return ((b0 & 0x0F) << 12) | ((b1 & 0x3F) << 6) | (b2 & 0x3F);
        }
    } else if (b0 >= 0xF0 && b0 <= 0xF4 && i + 3 < len) {
        unsigned char b1 = static_cast<unsigned char>(data[i + 1]);
        unsigned char b2 = static_cast<unsigned char>(data[i + 2]);
        unsigned char b3 = static_cast<unsigned char>(data[i + 3]);
        if ((b1 & 0xC0) == 0x80 && (b2 & 0xC0) == 0x80 && (b3 & 0xC0) == 0x80) {
            consumed = 4;
            return ((b0 & 0x07) << 18) | ((b1 & 0x3F) << 12) | ((b2 & 0x3F) << 6) | (b3 & 0x3F);
        }
    }
    consumed = 1;
    return b0;
}

static std::string htmlEscapeCfdump(const char *data, size_t len)
{
    std::string out;
    for (size_t i = 0; i < len;) {
        unsigned char c = static_cast<unsigned char>(data[i]);
        switch (c) {
        case '&': out += "&amp;"; i++; continue;
        case '<': out += "&lt;"; i++; continue;
        case '>': out += "&gt;"; i++; continue;
        case '\"': out += "&quot;"; i++; continue;
        case '\'': out += "&#x27;"; i++; continue;
        case '`': out += "&#x60;"; i++; continue;
        case '\t': out += "&#x9;"; i++; continue;
        case '\n': out += "&#xa;"; i++; continue;
        case '\r': out += "&#xd;"; i++; continue;
        case ';': out += "&#x3b;"; i++; continue;
        case '(': out += "&#x28;"; i++; continue;
        case ')': out += "&#x29;"; i++; continue;
        case '+': out += "&#x2b;"; i++; continue;
        case '=': out += "&#x3d;"; i++; continue;
        case '*': out += "&#x2a;"; i++; continue;
        case '/': out += "&#x2f;"; i++; continue;
        case '%': out += "&#x25;"; i++; continue;
        case '@': out += "&#x40;"; i++; continue;
        case '!': out += "&#x21;"; i++; continue;
        case '?': out += "&#x3f;"; i++; continue;
        case '^': out += "&#x5e;"; i++; continue;
        case '|': out += "&#x7c;"; i++; continue;
        case '~': out += "&#x7e;"; i++; continue;
        case '[': out += "&#x5b;"; i++; continue;
        case ']': out += "&#x5d;"; i++; continue;
        case '{': out += "&#x7b;"; i++; continue;
        case '}': out += "&#x7d;"; i++; continue;
        case ':': out += "&#x3a;"; i++; continue;
        case '#': out += "&#x23;"; i++; continue;
        case '$': out += "&#x24;"; i++; continue;
        case '\\': out += "&#x5c;"; i++; continue;
        default: break;
        }
        if (c < 0x80) {
            out += static_cast<char>(c);
            i++;
            continue;
        }
        size_t consumed = 1;
        int cp = cfdumpDecodeChar(data, len, i, consumed);
        if (const char *ent = html4NamedEntity(cp)) {
            out += ent;
        } else {
            char buf[16];
            std::snprintf(buf, sizeof(buf), "&#x%x;", cp);
            out += buf;
        }
        i += consumed;
    }
    return out;
}

static std::string cfdumpScalar(const cfvariant &v, bool html)
{
    switch (v.m_type) {
    case cfvariant::Null:
        return "undefined";
    case cfvariant::Boolean:
        return v.m_bool ? "true" : "false";
    case cfvariant::Number:
        return string::number(v.m_int).constData();
    case cfvariant::Long:
        return string::number(v.m_long).constData();
    case cfvariant::Float: {
        // CF preserves the literal text of float literals (8.10 -> "8.10");
        // only computed doubles use the %.12g renderer below.
        std::string s = v.m_literalText ? safe_to_std_string(v.m_literalText) : formatCfdumpFloat(v.m_double);
        if (html) return htmlEscapeCfdump(s.c_str(), s.size());
        return s;
    }
    case cfvariant::DateTime: {
        const webstrada::string s = const_cast<cfvariant&>(v).toString();
        std::string str = safe_to_std_string(s);
        if (html) return htmlEscapeCfdump(str.c_str(), str.size());
        return str;
    }
    case cfvariant::String: {
        if (!v.m_str || v.m_str->isEmpty()) {
            return html ? "&#x5b;empty string&#x5d;" : "";
        }
        const char *d = v.m_str->constData();
        if (html) return htmlEscapeCfdump(d, v.m_str->length());
        return d ? d : "";
    }
    default:
        return "";
    }
}

static std::string cfdumpBinaryString(const cfvariant &v)
{
    std::string out;
    if (v.m_binary) {
        for (const auto &b : *v.m_binary) {
            int byteVal = static_cast<int>(std::to_integer<unsigned char>(b));
            if (byteVal >= 128) byteVal -= 256;
            char buf[16];
            std::snprintf(buf, sizeof(buf), "%d", byteVal);
            out += buf;
        }
    }
    return out;
}

static std::string cfdumpHeader(const std::string &type, const std::string &label)
{
    if (label.empty()) return type;
    return label + " - " + type;
}

static std::string cfdumpNum(int n)
{
    return std::string(string::number(n).constData());
}

static std::string cfdumpHtmlValue(const cfvariant &v, const CfdumpOptions &opts, int depth);

static bool cfdumpIsContainer(const cfvariant &v)
{
    switch (v.m_type) {
    case cfvariant::Struct:
    case cfvariant::Array:
    case cfvariant::Binary:
    case cfvariant::Query:
    case cfvariant::Function:
        return true;
    default:
        return false;
    }
}

static std::string cfdumpHtmlStruct(const cfvariant &v, const CfdumpOptions &opts, bool topLevel, int depth)
{
    const void *skey = v.m_structData;
    bool isCycle = skey && !g_cfdumpVisited.insert(skey).second;
    std::string header = cfdumpHeader("struct", opts.label);
    bool empty = !v.m_struct || v.m_struct->empty();
    if (isCycle) return header + " [circular]";

    // Collect keys in sorted order (our struct map stores uppercased keys).
    std::vector<std::string> keys;
    if (v.m_struct) {
        for (const auto &pair : *v.m_struct) keys.push_back(pair.first.constData());
    }
    std::sort(keys.begin(), keys.end());
    int totalKeys = static_cast<int>(keys.size());

    // show / hide / keys filters (top level only).
    std::vector<std::string> filtered;
    if (topLevel && !empty) {
        if (!opts.show.empty()) {
            std::vector<std::string> shown;
            std::string tmp = opts.show;
            size_t start = 0;
            while (start <= tmp.size()) {
                size_t comma = tmp.find(',', start);
                if (comma == std::string::npos) comma = tmp.size();
                std::string k = tmp.substr(start, comma - start);
                std::string uk;
                for (char &ch : k) uk += (char)toupper((unsigned char)ch);
                if (!uk.empty()) shown.push_back(uk);
                start = comma + 1;
            }
            for (const auto &k : keys) {
                for (const auto &s : shown) {
                    if (s == k) { filtered.push_back(k); break; }
                }
            }
            header += " [Filtered - " + cfdumpNum(static_cast<int>(filtered.size())) + " of " +
                      cfdumpNum(totalKeys) + " keys shown]";
        } else if (!opts.hide.empty()) {
            std::vector<std::string> hidden;
            std::string tmp = opts.hide;
            size_t start = 0;
            while (start <= tmp.size()) {
                size_t comma = tmp.find(',', start);
                if (comma == std::string::npos) comma = tmp.size();
                std::string k = tmp.substr(start, comma - start);
                std::string uk;
                for (char &ch : k) uk += (char)toupper((unsigned char)ch);
                if (!uk.empty()) hidden.push_back(uk);
                start = comma + 1;
            }
            for (const auto &k : keys) {
                bool isHidden = false;
                for (const auto &h : hidden) if (h == k) { isHidden = true; break; }
                if (!isHidden) filtered.push_back(k);
            }
            int hiddenCount = 0;
            for (const auto &k : keys) {
                for (const auto &h : hidden) if (h == k) { hiddenCount++; break; }
            }
            header += " [Filtered - " + cfdumpNum(hiddenCount) + " of " +
                      cfdumpNum(totalKeys) + " keys hidden]";
        } else if (opts.keys >= 0 && opts.keys < totalKeys) {
            keys.resize(opts.keys);
            header += " [Filtered - top " + cfdumpNum(opts.keys) + " of " +
                      cfdumpNum(totalKeys) + " keys shown]";
        }
        if (!opts.show.empty() || !opts.hide.empty()) keys = filtered;
    }

    std::string out;
    if (empty) {
        if (skey) g_cfdumpVisited.erase(skey);
        out += "\r\n\t\t\t<table class=\"cfdump_struct\">\r\n\t\t\t<tr><th class=\"struct\" colspan=\"2\">"
               + header + " [empty]</th></tr> \r\n\t\t\t</table>\r\n\t\t";
        return out;
    }

    out += "\r\n\t\t\t<table class=\"cfdump_struct\">\r\n\t\t\t<tr><th class=\"struct\" colspan=\"2\">"
           + header + "</th></tr> \r\n";
    for (const auto &key : keys) {
        const cfvariant &val = v.m_struct->at(key.c_str());
        std::string valHtml = cfdumpHtmlValue(val, opts, depth + 1);
        out += "\t\t\t\t\t<tr>\r\n\t\t\t\t\t<td class=\"struct\">" + key + "</td>\r\n\t\t\t\t\t<td>\r\n\t\t\t\t\t";
        if (cfdumpIsContainer(val)) {
            // Container: blank spacer line, then the container's own template,
            // then the cell closes on its own 5-tab line.
            out += valHtml + "\r\n\t\t\t\t\t</td>\r\n\t\t\t\t\t</tr>\r\n\t\t\t\t\t\r\n";
        } else {
            out += valHtml + " \r\n\t\t\t\t\t</td>\r\n\t\t\t\t\t</tr>\r\n\t\t\t\t\t\r\n";
        }
    }
    out += "\t\t\t</table>\r\n\t\t";
    if (skey) g_cfdumpVisited.erase(skey);
    return out;
}

static std::string cfdumpHtmlArray(const cfvariant &v, const CfdumpOptions &opts, bool topLevel, int depth)
{
    const void *akey = v.m_array;
    bool isCycle = akey && !g_cfdumpVisited.insert(akey).second;
    std::string header = cfdumpHeader("array", opts.label);
    bool empty = !v.m_array || v.m_array->empty();
    if (isCycle) return header + " [circular]";
    size_t total = v.m_array ? v.m_array->size() : 0;

    size_t count = total;
    if (topLevel && opts.top >= 0 && static_cast<size_t>(opts.top) < count) {
        count = static_cast<size_t>(opts.top);
    }

    std::string out;
    if (empty) {
        if (akey) g_cfdumpVisited.erase(akey);
        out += "\r\n\t\t\t\t<table class=\"cfdump_array\">\r\n\t\t\t\t<tr><th class=\"array\" colspan=\"2\">"
               + header + "[empty]\r\n\t\t\t\t</th></tr>\r\n\t\t\t\t\r\n\t\t\t</table>\r\n\t\t";
        return out;
    }

    out += "\r\n\t\t\t\t<table class=\"cfdump_array\">\r\n\t\t\t\t<tr><th class=\"array\" colspan=\"2\">"
           + header;
    if (topLevel && opts.top >= 0 && static_cast<size_t>(opts.top) < total) {
        out += " - Top " + cfdumpNum(opts.top) + " of " +
               cfdumpNum(static_cast<int>(total)) + " rows";
    }
    out += "\r\n\t\t\t\t</th></tr>\r\n\t\t\t\t\r\n";

    for (size_t i = 0; i < count; i++) {
        const cfvariant &item = v.m_array->at(i);
        std::string itemHtml = cfdumpHtmlValue(item, opts, depth + 1);
        out += "\t\t\t\t\t<tr><td class=\"array\">" + cfdumpNum(static_cast<int>(i + 1)) + "</td>\r\n\t\t\t\t\t<td> ";
        if (cfdumpIsContainer(item)) {
            // Container: its own fixed template, then the cell closes inline.
            out += itemHtml + "</td></tr> \r\n";
        } else {
            out += itemHtml + " </td></tr> \r\n";
        }
    }
    out += "\t\t\t</table>\r\n\t\t";
    if (akey) g_cfdumpVisited.erase(akey);
    return out;
}

static std::string cfdumpHtmlBinary(const cfvariant &v, const CfdumpOptions &opts)
{
    std::string out;
    out += "\r\n\t\t\t<table class=\"cfdump_binary\">\r\n\t\t\t<tr><th class=\"binary\">"
           + cfdumpHeader("binary", opts.label) + "</th></tr>\r\n\t\t\t<tr><td class=\"binary\">\r\n\t\t\t<code>"
           + cfdumpBinaryString(v) + "</code>\r\n\t\t\t\r\n\t\t\t</td></tr></table>\r\n\t\t\t";
    return out;
}

static std::string cfdumpHtmlQuery(const cfvariant &v, const CfdumpOptions &opts)
{
    // Sorted column order (case-insensitive) kept as indices into
    // v.m_query->columns so headers and cells stay aligned for unsorted input.
    std::vector<int> colOrder;
    if (v.m_query) {
        for (size_t i = 0; i < v.m_query->columns.size(); i++) colOrder.push_back(static_cast<int>(i));
        std::sort(colOrder.begin(), colOrder.end(), [&](int a, int b) {
            return strcasecmp(v.m_query->columns[a].name.constData(), v.m_query->columns[b].name.constData()) < 0;
        });
    }

    int colspan = static_cast<int>(colOrder.size()) + 1;
    std::string out;
    out += "\r\n\t\t<table class=\"cfdump_query\">\r\n\t\t\t<tr>\r\n\t\t\t\r\n\t\t\t\r\n\t\t\t\t<th class=\"query\" colspan=\""
           + cfdumpNum(colspan) + "\">" + cfdumpHeader("query", opts.label) + "</th>\r\n\t\t\t\t</tr>\r\n\t\t\t";
    if (!colOrder.empty()) {
        out += "\r\n\t\t\t\t<tr bgcolor=\"eeaaaa\" >\r\n\t\t\t\t<td class=\"query\"  onClick=\"cfdump_toggleRow_qry(this);\">&nbsp;</td>\r\n\t\t\t\t";
        for (int ci : colOrder) {
            out += "\r\n\t\t\t\t<td class=\"query\">" + std::string(v.m_query->columns[ci].name.constData()) + "</td>\r\n\t\t\t\t";
        }
        out += "\r\n\t\t\t\t</tr>\r\n\t\t\t\t\r\n\t\t\t\t";
    }
    if (v.m_query) {
        int rows = v.m_query->rowCount();
        for (int r = 0; r < rows; r++) {
            out += "\r\n\t\t\t\t<tr >\r\n\t\t\t\t<td  onClick=\"cfdump_toggleRow_qry(this);\" class=\"query\">"
                   + cfdumpNum(r + 1) + "</td>\r\n\t\t\t\t\r\n\t\t\t\t\r\n";
            for (int ci : colOrder) {
                const cfvariant &cell = v.m_query->columns[ci].values[r];
                std::string cellHtml = cfdumpScalar(cell, false);
                out += "\t\t\t\t\t<td valign=\"top\">\r\n\t\t\t\t\t" + cellHtml + " \r\n\t\t\t\t\t</td>\r\n\t\t\t\t\r\n";
            }
            out += "\t\t\t\t</tr>\r\n\t\t\t\t";
        }
    }
    out += "\r\n\t\t</table>\r\n\t\t";
    return out;
}

static std::string cfdumpHtmlUdf(const cfvariant &v, const CfdumpOptions &opts, int depth)
{
    // CF quirk (verified byte-for-byte against CF 2021): the very first UDF
    // rendered in the page's first (style-emitting) dump carries a leading
    // space; so does the first UDF at each nesting depth >= 2 anywhere on the
    // page. Depth-1 UDFs in later dumps never do.
    bool leadingSpace = false;
    if (g_cfdump_udf_first_space && !g_cfdump_style_emitted) {
        leadingSpace = true;
        g_cfdump_udf_first_space = false;
    }
    if (depth >= 2 && g_cfdump_udf_seen_depths.count(depth) == 0) {
        leadingSpace = true;
        g_cfdump_udf_seen_depths.insert(depth);
    }

    std::string name;
    std::string kind = "function";
    std::vector<UdfParamInfo> params;
    std::string returnType = "Any";
    std::string access = "public";
    if (v.m_udf) {
        name = safe_to_std_string(v.m_udf->name);
        kind = v.m_udf->isClosure ? "closure" : "function";
        params = v.m_udf->params;
        if (!v.m_udf->returnType.isEmpty()) returnType = safe_to_std_string(v.m_udf->returnType);
        if (!v.m_udf->access.isEmpty()) access = safe_to_std_string(v.m_udf->access);
    } else {
        // Built-in method handle: display its identity string.
        if (v.m_str) name = safe_to_std_string(*v.m_str);
    }

    std::string out;
    if (leadingSpace) out += " ";
    out += "\r\n\t\t<table class=\"cfdump_udf\" width=\"100%\">\r\n";
    out += "\t\t<tr><th class=\"udf\" colspan=\"2\">" + kind + " " + name + "</b></th></tr>\r\n";
    out += "\t\t<tr>\r\n";
    out += "\t\t\t<td>\r\n";
    out += "\t\t\t<table class=\"cfdump_udfbody\">\r\n";
    out += "\t\t\t<tr>\r\n";
    out += "\t\t\t\t\r\n";
    out += "\t\t\t\t<td colspan=\"2\">\r\n";
    out += "\t\t\t\t<i>Arguments:</i>\r\n";
    out += "\t\t\t\t<br>\r\n";
    out += "\t\t\t\t<table class=\"cfdump_udfarguments\">\r\n";
    out += "\t\t\t\t\t<tr>\r\n";
    out += "\t\t\t\t\t\t<th><b>Name</b></th>\r\n";
    out += "\t\t\t\t\t\t<th><b>Required</b></th>\r\n";
    out += "\t\t\t\t\t\t<th><b>Type</b></th>\r\n";
    out += "\t\t\t\t\t\t<th><b>Default</b></th>\r\n";
    out += "\t\t\t\t\t</tr>\r\n";
    out += "\t\t\t\t\t\r\n";
    for (const auto &param : params) {
        std::string type = param.type.isEmpty() ? "Any" : safe_to_std_string(param.type);
        std::string def = param.defaultValue.isEmpty() ? "&nbsp;" : safe_to_std_string(param.defaultValue) + " ";
        out += "\t\t\t\t\t<tr>\r\n";
        out += "\t\t\t\t\t\t<td>" + safe_to_std_string(param.name) + "</td>\r\n";
        out += "\t\t\t\t\t\t<td>Optional</td>\r\n";
        out += "\t\t\t\t\t\t<td>" + type + "</td>\r\n";
        out += "\t\t\t\t\t\t<td>" + def + "</td>\r\n";
        out += "\t\t\t\t\t</tr>\r\n";
        out += "\t\t\t\t\t\r\n";
    }
    out += "\t\t\t\t</table>\r\n";
    out += "\t\t\t\t\r\n";
    out += "\t\t\t</tr>\r\n";
    out += "\t\t\t<tr><td width=\"30%\"><i>ReturnType:</i></td>\r\n";
    out += "\t\t\t\t<td>" + returnType + "<br></td>\r\n";
    out += "\t\t\t</tr>\r\n";
    out += "\t\t\t<tr><td><i>Roles:</i></td>\r\n";
    out += "\t\t\t\t<td>&nbsp;<br></td>\r\n";
    out += "\t\t\t</tr>\r\n";
    out += "\t\t\t<tr><td><i>Access:</i></td>\r\n";
    out += "\t\t\t\t<td>" + access + "</br></td>\r\n";
    out += "\t\t\t</tr>\r\n";
    out += "\t\t\t<tr><td><i>Static:</i></td>\r\n";
    out += "\t\t\t\t<td>false</br></td>\r\n";
    out += "\t\t\t</tr>\r\n";
    out += "\t\t\t<tr><td><i>Output:</i></td>\r\n";
    out += "\t\t\t\t<td>&nbsp;</td>\r\n";
    out += "\t\t\t</tr>\r\n";
    out += "\t\t\t<tr><td><i>DisplayName:</i></td>\r\n";
    out += "\t\t\t\t<td>&nbsp;</td>\r\n";
    out += "\t\t\t</tr>\r\n";
    out += "\t\t\t<tr><td><i>Hint:</i></td>\r\n";
    out += "\t\t\t\t<td>&nbsp;</td>\r\n";
    out += "\t\t\t</tr>\r\n";
    out += "\t\t\t<tr><td><i>Description:</i></td>\r\n";
    out += "\t\t\t\t<td>&nbsp;</td>\r\n";
    out += "\t\t\t</tr>\r\n";
    out += "\t\t\t</table>\r\n";
    out += "\t\t\t</td>\r\n";
    out += "\t\t</tr>\r\n";
    out += "\t\t</table>\r\n";
    out += "\t\t";
    return out;
}

static std::string cfdumpHtmlValue(const cfvariant &v, const CfdumpOptions &opts, int depth)
{
    switch (v.m_type) {
    case cfvariant::Struct:
        return cfdumpHtmlStruct(v, opts, false, depth);
    case cfvariant::Array:
        return cfdumpHtmlArray(v, opts, false, depth);
    case cfvariant::Binary:
        return cfdumpHtmlBinary(v, opts);
    case cfvariant::Query:
        return cfdumpHtmlQuery(v, opts);
    case cfvariant::Function:
        return cfdumpHtmlUdf(v, opts, depth);
    default:
        return cfdumpScalar(v, true);
    }
}

static std::string cfdumpTextValue(const cfvariant &v, int depth);

static std::string cfdumpTextArrayItemValue(const cfvariant &v, int depth);

static std::string cfdumpTextItems(const cfvariant &arr, int depth)
{
    std::string out;
    size_t n = arr.m_array->size();
    std::string indent(depth, '\t');
    for (size_t i = 0; i < n; i++) {
        const cfvariant &item = arr.m_array->at(i);
        bool isLast = (i == n - 1);
        std::string body = indent + cfdumpNum(static_cast<int>(i + 1)) + ") ";
        if (item.m_type == cfvariant::Array || item.m_type == cfvariant::Struct ||
            item.m_type == cfvariant::Function) {
            out += body + cfdumpTextArrayItemValue(item, depth + 1);
        } else {
            int pad = 1 + (isLast ? depth : 0);
            out += body + cfdumpScalar(item, false) + std::string(pad, ' ') + "\r\n";
        }
    }
    return out;
}

static std::string cfdumpTextKeys(const cfvariant &st, int depth, bool padLast)
{
    std::string out;
    std::vector<std::string> keys;
    if (st.m_struct) {
        for (const auto &pair : *st.m_struct) keys.push_back(pair.first.constData());
    }
    std::sort(keys.begin(), keys.end());
    size_t n = keys.size();
    std::string indent(depth, '\t');
    for (size_t i = 0; i < n; i++) {
        const cfvariant &val = st.m_struct->at(keys[i].c_str());
        bool isLast = (i == n - 1);
        std::string body = indent + keys[i] + ": ";
        if (val.m_type == cfvariant::Array || val.m_type == cfvariant::Struct ||
            val.m_type == cfvariant::Function) {
            // Container values inside structs get a leading space + CRLF before
            // the [type] marker (verified against CF 2021).
            out += body + " \r\n" + std::string(depth + 1, '\t') + cfdumpTextValue(val, depth + 1);
        } else {
            out += body + cfdumpScalar(val, false);
            if (padLast && isLast) out += std::string(depth, ' ');
            out += "\r\n";
        }
    }
    return out;
}

static std::string cfdumpTextUdfBody(const cfvariant &v, int depth, bool padLast)
{
    std::string indent(depth, '\t');
    std::string out;
    out += indent + "Arguments: \r\n";
    std::vector<UdfParamInfo> params;
    if (v.m_udf) params = v.m_udf->params;
    for (size_t i = 0; i < params.size(); i++) {
        const UdfParamInfo &param = params[i];
        out += indent + "\tName: " + safe_to_std_string(param.name) + " \r\n";
        out += indent + "\tRequired: Optional \r\n";
        std::string type = param.type.isEmpty() ? "Any" : safe_to_std_string(param.type);
        out += indent + "\tType: " + type + " \r\n";
        if (param.defaultValue.isEmpty()) {
            out += indent + "\tdefault: \r\n";
        } else {
            out += indent + "\tdefault: " + safe_to_std_string(param.defaultValue) + "\r\n";
        }
        if (i + 1 < params.size()) out += indent + " \r\n";
    }
    std::string returnType = "Any";
    if (v.m_udf && !v.m_udf->returnType.isEmpty()) returnType = safe_to_std_string(v.m_udf->returnType);
    std::string access = "public";
    if (v.m_udf && !v.m_udf->access.isEmpty()) access = safe_to_std_string(v.m_udf->access);
    out += indent + "ReturnType: " + returnType + " \r\n";
    out += indent + "Roles:  \r\n";
    out += indent + "Access: " + access + " \r\n";
    out += indent + "Static: false \r\n";
    out += indent + "Output:   \r\n";
    out += indent + "DisplayName:  \r\n";
    out += indent + "Hint:  \r\n";
    out += indent + "Description:  ";
    if (padLast) out += std::string(depth, ' ');
    out += "\r\n";
    return out;
}

static std::string cfdumpTextValue(const cfvariant &v, int depth)
{
    if (v.m_type == cfvariant::Function) {
        // The caller (cfdumpTextKeys) prefixes the indent; like the struct/array
        // cases, the marker itself carries no leading tabs.
        std::string out = "[function]\r\n";
        out += cfdumpTextUdfBody(v, depth, false);
        return out;
    }
    if (v.m_type == cfvariant::Struct) {
        const void *skey = v.m_structData;
        if (skey && !g_cfdumpVisited.insert(skey).second) return "[circular]\r\n";
        std::string out = "[struct]\r\n";
        out += cfdumpTextKeys(v, depth, false);
        if (skey) g_cfdumpVisited.erase(skey);
        return out;
    }
    if (v.m_type == cfvariant::Array) {
        const void *akey = v.m_array;
        if (akey && !g_cfdumpVisited.insert(akey).second) return "[circular]\r\n";
        std::string out = "[array]\r\n";
        out += cfdumpTextItems(v, depth);
        if (akey) g_cfdumpVisited.erase(akey);
        return out;
    }
    if (v.m_type == cfvariant::Query && v.m_query) {
        // Text dump of a query: per-record rows, byte-matched against CF 2021.
        std::vector<int> colOrder;
        for (size_t i = 0; i < v.m_query->columns.size(); i++) colOrder.push_back(static_cast<int>(i));
        std::stable_sort(colOrder.begin(), colOrder.end(), [&](int a, int b) {
            return strcasecmp(v.m_query->columns[a].name.constData(), v.m_query->columns[b].name.constData()) < 0;
        });
        std::string out = "[query]\r\n";
        int rows = v.m_query->rowCount();
        std::string indent(depth, '\t');
        for (int r = 0; r < rows; r++) {
            out += indent + " \r\n" + indent + "[Record # " + cfdumpNum(r + 1) + "] \r\n";
            for (size_t ci = 0; ci < colOrder.size(); ci++) {
                size_t c = static_cast<size_t>(colOrder[ci]);
                const cfvariant &cell = v.m_query->columns[c].values[r];
                std::string cellText = cfdumpScalar(cell, false);
                bool isLast = (ci == colOrder.size() - 1);
                out += indent + std::string(v.m_query->columns[c].name.constData()) + ": " + cellText;
                if (isLast) {
                    out += "\r\n";
                } else {
                    out += " \r\n";
                }
            }
        }
        return out;
    }
    return cfdumpScalar(v, false);
}

static std::string cfdumpTextArrayItemValue(const cfvariant &v, int depth)
{
    if (v.m_type == cfvariant::Function) {
        std::string out = "[function]\r\n";
        out += cfdumpTextUdfBody(v, depth, true);
        return out;
    }
    if (v.m_type == cfvariant::Struct) {
        const void *skey = v.m_structData;
        if (skey && !g_cfdumpVisited.insert(skey).second) return "[circular]\r\n";
        std::string out = "[struct]\r\n";
        out += cfdumpTextKeys(v, depth, true);
        if (skey) g_cfdumpVisited.erase(skey);
        return out;
    }
    if (v.m_type == cfvariant::Array) {
        const void *akey = v.m_array;
        if (akey && !g_cfdumpVisited.insert(akey).second) return "[circular]\r\n";
        std::string out = "[array]\r\n";
        out += cfdumpTextItems(v, depth);
        if (akey) g_cfdumpVisited.erase(akey);
        return out;
    }
    return cfdumpScalar(v, false);
}

static std::string cfdumpText(const cfvariant &v, const std::string &label)
{
    if (v.m_type == cfvariant::Struct) {
        bool empty = !v.m_struct || v.m_struct->empty();
        std::string out = "<pre>" + cfdumpHeader("struct", label) + (empty ? " [empty]" : "") + "\r\n\r\n";
        if (!empty) {
            const void *skey = v.m_structData;
            if (skey && !g_cfdumpVisited.insert(skey).second) {
                out += "[circular]";
            } else {
                out += cfdumpTextKeys(v, 0, false);
                if (skey) g_cfdumpVisited.erase(skey);
            }
        }
        out += "</pre>";
        return out;
    }
    if (v.m_type == cfvariant::Array) {
        bool empty = !v.m_array || v.m_array->empty();
        std::string out = "<pre>" + cfdumpHeader("array", label) + (empty ? "[empty]" : "") + "\r\n\r\n";
        if (!empty) {
            const void *akey = v.m_array;
            if (akey && !g_cfdumpVisited.insert(akey).second) {
                out += "[circular]";
            } else {
                out += cfdumpTextItems(v, 0);
                if (akey) g_cfdumpVisited.erase(akey);
            }
        }
        out += "</pre>";
        return out;
    }
    if (v.m_type == cfvariant::Binary) {
        return "<pre>" + cfdumpHeader("binary", label) + "\r\n\r\n" + cfdumpBinaryString(v) + "</pre>";
    }
    if (v.m_type == cfvariant::Query && v.m_query) {
        bool empty = v.m_query->rowCount() == 0;
        std::string out = "<pre>" + cfdumpHeader("query", label) + "\r\n\r\n";
        if (!empty) {
            std::vector<int> colOrder;
            for (size_t i = 0; i < v.m_query->columns.size(); i++) colOrder.push_back(static_cast<int>(i));
            std::stable_sort(colOrder.begin(), colOrder.end(), [&](int a, int b) {
                return strcasecmp(v.m_query->columns[a].name.constData(), v.m_query->columns[b].name.constData()) < 0;
            });
            int rows = v.m_query->rowCount();
            for (int r = 0; r < rows; r++) {
                out += " \r\n[Record # " + cfdumpNum(r + 1) + "] \r\n";
                for (size_t ci = 0; ci < colOrder.size(); ci++) {
                    size_t c = static_cast<size_t>(colOrder[ci]);
                    const cfvariant &cell = v.m_query->columns[c].values[r];
                    std::string cellText = cfdumpScalar(cell, false);
                    bool isLast = (ci == colOrder.size() - 1);
                    out += std::string(v.m_query->columns[c].name.constData()) + ": " + cellText;
                    out += isLast ? std::string("\r\n") : std::string(" \r\n");
                }
            }
        }
        out += "</pre>";
        return out;
    }
    if (v.m_type == cfvariant::Function) {
        std::string kind = "function";
        std::string name;
        if (v.m_udf) {
            kind = v.m_udf->isClosure ? "closure" : "function";
            name = safe_to_std_string(v.m_udf->name);
        } else if (v.m_str) {
            name = safe_to_std_string(*v.m_str);
        }
        return "<pre>" + kind + " " + name + "\r\n\r\n" + cfdumpTextUdfBody(v, 0, false) + "</pre>";
    }
    return "<pre>" + cfdumpScalar(v, false) + "</pre>";
}

cfvariant *cf_writedump(const cfvariant *var, const cfvariant *output, const cfvariant *format, const cfvariant *abort, const cfvariant *label, const cfvariant *metainfo, const cfvariant *top, const cfvariant *show, const cfvariant *hide, const cfvariant *keys, const cfvariant *expand, const cfvariant *showUDFs) {
    if (!var) throw webstrada::exception("WriteDump: Missing argument");

    g_cfdumpVisited.clear();
    CfdumpOptions opts;
    if (label) opts.label = safe_to_std_string(*label);
    if (top) opts.top = const_cast<cfvariant*>(top)->toString().toInt();
    if (keys) opts.keys = const_cast<cfvariant*>(keys)->toString().toInt();
    if (show) opts.show = safe_to_std_string(*show);
    if (hide) opts.hide = safe_to_std_string(*hide);

    bool textFormat = false;
    if (format) {
        webstrada::string fmt = const_cast<cfvariant*>(format)->toString();
        textFormat = fmt.equals("text");
    }

    std::string body;
    if (textFormat) {
        body = cfdumpText(*var, opts.label);
    } else {
        std::string content;
        switch (var->m_type) {
        case cfvariant::Struct:
            content = cfdumpHtmlStruct(*var, opts, true, 0);
            break;
        case cfvariant::Array:
            content = cfdumpHtmlArray(*var, opts, true, 0);
            break;
        case cfvariant::Binary:
            content = cfdumpHtmlBinary(*var, opts);
            break;
        case cfvariant::Query:
            content = cfdumpHtmlQuery(*var, opts);
            break;
        case cfvariant::Function:
            content = cfdumpHtmlUdf(*var, opts, 0);
            break;
        default:
            content = cfdumpScalar(*var, true) + " ";
            break;
        }
        if (!g_cfdump_style_emitted) {
            g_cfdump_style_emitted = true;
            body = cfdumpStyle() + " " + content;
        } else {
            body = content;
        }
    }

    if (abort && cfml::cfvariant_is_truthy(abort)) {
        g_cfdump_abort_pending = true;
    }

    auto *ret = new cfvariant(string(body.c_str()));
    // cf_writedump is called by the <cfdump> tag (emitCall) and directly by the
    // unit tests; register the result so every caller frees it via the request
    // cleanup (idempotent: the emitCall whitelist also registers it).
    cf_register_temp(ret);
    return ret;
}

} // namespace cfml
