#pragma once

// Locale data used by SetLocale/GetLocale and the localization-sensitive
// (LS*) functions. The table lives in src/locale_table.inc (generated from
// Adobe ColdFusion 2025 RDS host probes) and is indexed/looked up via the
// helpers in cf8.cpp.

namespace cfml {

struct LocaleInfo {
    const char *cfName;          // CF display name, e.g. "English (US)"
    const char *language;        // "en"
    const char *country;         // "US"
    const char *months[12];      // long month names
    const char *monthsShort[12]; // short month names
    const char *days[7];         // long day names, Sunday-first (tm_wday 0)
    const char *daysShort[7];    // short day names, Sunday-first
    const char *am, *pm;         // AM/PM markers
    const char *dateShort, *dateMedium, *dateLong, *dateFull; // CF-mask-language patterns
    const char *timeShort, *timeMedium;
    const char *numGroupSep, *numDecSep; // UTF-8 number separators
    const char *curGroupSep, *curDecSep; // UTF-8 currency separators
    int curDecimals;                     // 0 (JPY/KRW) or 2
    const char *curSymbol;               // local currency symbol (UTF-8)
    const char *curIntl;                 // international code, e.g. "USD"
    const char *curPattern;              // positive pattern with \u00A4 for symbol
    const char *curNegPattern;           // negative pattern with \u00A4 for symbol
    bool standaloneMonthCapitalized;     // bare "mmmm" mask capitalizes the month
                                        // (CF: Italian only — its CLDR standalone
                                        // month form is capitalized, e.g. Maggio)

    // Java SimpleDateFormat patterns (JDK COMPAT locale data) used by the
    // locale-aware date/time PARSER (LSParseDateTime / LSIsDate). CF 2025 runs
    // with java.locale.providers=COMPAT,SPI, so ParseDateTime builds its
    // formatter list from DateFormat.getDateInstance/getTimeInstance styles,
    // which yield these classic JDK patterns (NOT the CF-mask patterns above).
    // The parse month/day/AM-PM names come from the same COMPAT symbols, which
    // this table already stores in months/monthsShort/days/daysShort/am/pm.
    const char *pDateShort, *pDateMedium, *pDateLong, *pDateFull; // date patterns
    const char *pTimeShort, *pTimeMedium, *pTimeLong, *pTimeFull; // time patterns
};

// Look up a locale by CF display name or java locale code ("en_US", "fr", ...).
// Case-insensitive. Returns nullptr when not found.
const LocaleInfo *locale_find(const char *name);

// The locale that formatting falls back to when none is set ("English (US)").
const LocaleInfo *locale_default();

// The CF string (GetLocale / SetLocale return value) for a locale: the display
// name for named locales, the java code otherwise.
const char *locale_cf_string(const LocaleInfo *loc);

} // namespace cfml
