/**
 * @file fn_getlocaledisplayname.cpp
 * @brief CFML getlocaledisplayname() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>
#include <webstrada/exceptions.h>
#include <webstrada/locale.h>
#include <webstrada/string.h>
#include <string>

namespace cfml {

namespace {

// The 28x28 display-name matrix (each locale's name as shown in every other
// locale's language), probed byte-for-byte from the CF 2025 RDS host. CF
// implements GetLocaleDisplayName as Locale.getDisplayName(inLocale) with the
// JDK's COMPAT locale provider (older JDK data), so the names here are the
// authoritative values (e.g. "Norwegian (Norway,Nynorsk)").
struct LdnEntry {
    const char *locale;
    const char *inLocale;
    const char *name;
};
#include "ldn_table.inc"

const char *lookupDisplayName(const std::string &locale, const std::string &inLocale) {
    for (const auto &e : kLdnEntries) {
        if (locale == e.locale && inLocale == e.inLocale) return e.name;
    }
    return nullptr;
}

} // namespace

cfvariant *cf_getlocaledisplayname(const cfvariant *locale, const cfvariant *inLocale) {
    // Current request locale. CF's default locale is the bare "en" (GetLocale
    // returns "en"); the engine stores "en_US" for the unset default, which
    // resolves to the same "English (US)" locale for formatting but must render
    // the CF default display name "English" here.
    std::string cur = currentLocaleStr() ? currentLocaleStr() : "en_US";
    std::string loc = cur;
    std::string in = cur;

    bool localeExplicit = locale != nullptr && locale->m_type != cfvariant::Null;
    bool inLocaleExplicit = inLocale != nullptr && inLocale->m_type != cfvariant::Null;

    if (localeExplicit) {
        std::string l = const_cast<cfvariant*>(locale)->toString().constData();
        const cfml::LocaleInfo *li = cfml::locale_find(l.c_str());
        if (li) l = li->cfName;
        loc = l;
    } else if (cur == "en_US") {
        // CF's default locale is the bare "en"; its self display name is
        // "English" (not "English (United States)").
        loc = "en";
    }
    if (inLocaleExplicit) {
        std::string l = const_cast<cfvariant*>(inLocale)->toString().constData();
        const cfml::LocaleInfo *li = cfml::locale_find(l.c_str());
        if (li) l = li->cfName;
        in = l;
    } else if (cur == "en_US") {
        in = "en";
    }

    // CF's default locale is the bare "en": the display names are the English
    // names (which equal the en_US names for every CF locale). When the
    // resolved in-locale is the bare "en", use the English display names by
    // falling back to the en_US column.
    const char *name = lookupDisplayName(loc, in);
    if (!name && in == "en") name = lookupDisplayName(loc, "English (US)");
    if (!name) name = lookupDisplayName(loc, loc); // fall back to self-name
    if (!name) name = loc.c_str();
    return new cfvariant(name);
}

} // namespace cfml
