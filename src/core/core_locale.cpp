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
// ---- Locale registry ----
// ---- Locale registry (<cfsetlocale>/<cfgetlocale> and the LS* functions) ----

#include "../src/locale_table.inc"

static const cfml::LocaleInfo *localeFindCaseInsensitive(const char *name)
{
    if (!name || !*name) return nullptr;
    webstrada::string n(name);
    n.toLower();
    for (const auto &loc : kLocales) {
        webstrada::string cn(loc.cfName);
        cn.toLower();
        if (cn.equals(n)) return &loc;
    }
    // java locale code ("en_US", "fr", "zh_HK") or language-only: match by
    // language + country (a missing country matches the canonical country).
    webstrada::string lang, country, variant;
    int first = n.indexOf('_');
    if (first < 0) {
        lang = n;
    } else {
        lang = n.left(first);
        webstrada::string rest = n.mid(first + 1, n.length() - first - 1);
        int second = rest.indexOf('_');
        if (second < 0) {
            country = rest;
        } else {
            country = rest.left(second);
            variant = rest.mid(second + 1, rest.length() - second - 1);
        }
    }
    for (const auto &loc : kLocales) {
        webstrada::string l(loc.language);
        webstrada::string c(loc.country);
        l.toLower();
        c.toLower();
        if (l.equals(lang) && (country.isEmpty() || c.equals(country))) return &loc;
    }
    return nullptr;
}

const cfml::LocaleInfo *cfml::locale_find(const char *name)
{
    return localeFindCaseInsensitive(name);
}

const cfml::LocaleInfo *cfml::locale_default()
{
    static const cfml::LocaleInfo *enUS = localeFindCaseInsensitive("en_US");
    return enUS ? enUS : &kLocales[0];
}

const char *cfml::locale_cf_string(const cfml::LocaleInfo *loc)
{
    return loc ? loc->cfName : "en_US";
}


