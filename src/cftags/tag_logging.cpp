/**
 * @file tag_logging.cpp
 * @brief <cftimer> runtime helpers (cf_timer_begin / cf_timer_end).
 *
 * <cflog> shares the WriteLog runtime (cf_writelog, src/core/core_misc.cpp)
 * and <cftrace> is a complete no-op when debugging is disabled, so this file
 * only implements the timer.
 *
 * <cftimer> always evaluates its body; the timing display (inline/comment/
 * outline) is gated on the debugging service exactly like CF's TimerTag:
 * with debugging disabled (the default on the RDS verification host and in
 * this engine, which has no debug output section) no timing is displayed.
 * The `type` attribute is validated at runtime — an invalid value is a
 * catchable Template error with CF's IllegalSwitchValueException message,
 * even for a static literal (verified on CF 2025: <cftimer type="bogus">
 * inside <cftry> is caught, type="Template", detail "The value of the TYPE
 * attribute, which is currently bogus, must be one of the values:
 * DEBUG,OUTLINE,INLINE,COMMENT."; an empty value renders '').
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>
#include <webstrada/config.h>
#include <webstrada/exceptions.h>
#include <webstrada/string.h>

#include <chrono>
#include <cctype>
#include <string>

namespace cfml {

using webstrada::cfvariant;
using webstrada::string;

static int64_t nowMillis()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

// Validates the `type` attribute (default "debug") and returns the start
// timestamp. An invalid type throws CF's catchable Template error.
int64_t cf_timer_begin(const cfvariant *type)
{
    std::string t = "debug";
    if (type) t = safe_to_std_string(*type);
    std::string up;
    for (char c : t) up.push_back(static_cast<char>(toupper((unsigned char)c)));
    if (up != "DEBUG" && up != "OUTLINE" && up != "INLINE" && up != "COMMENT") {
        std::string shown = t.empty() ? "''" : t;
        throw webstrada::exception("Template",
            webstrada::string("Attribute validation error for CFTIMER."),
            webstrada::string(("The value of the TYPE attribute, which is currently " +
                              shown + ", must be one of the values: "
                              "DEBUG,OUTLINE,INLINE,COMMENT.").c_str()));
    }
    return nowMillis();
}

// Measures the elapsed time and, when debugging is enabled, emits the timing
// per CF's TimerTag.doEndTag. This engine has no debug section
// (config::debugEnabled is false), so nothing is written — the tag's body has
// already been evaluated by the compiled template.
void cf_timer_end(int64_t start, const cfvariant *type, const cfvariant *label, void *out)
{
    if (!webstrada::config::debugEnabled) return;
    int64_t elapsed = nowMillis() - start;
    std::string t = type ? safe_to_std_string(*type) : "debug";
    std::string lbl = label ? safe_to_std_string(*label) : "cftimer";
    std::string up;
    for (char c : t) up.push_back(static_cast<char>(toupper((unsigned char)c)));
    std::string text = lbl + ": " + std::to_string(elapsed) + "ms";
    if (up == "INLINE") {
        if (out) static_cast<string*>(out)->append(text.c_str());
    } else if (up == "COMMENT") {
        std::string c = "<!-- " + text + " -->";
        if (out) static_cast<string*>(out)->append(c.c_str());
    } else if (up == "OUTLINE") {
        std::string f =
            "<fieldset class='cftimer'>\n   <legend align='top'></legend>\n"
            "<script language='JavaScript'>\n"
            "   if (document.getElementById) {\n"
            "       document.getElementById('').innerHTML = '" + text + "';\n"
            "   }else{\n"
            "       document.write('" + text + "');\n"
            "   }\n"
            "</script>\n</fieldset>\n";
        if (out) static_cast<string*>(out)->append(f.c_str());
    }
    // type="debug": the timing belongs in the debug output section, which this
    // engine does not render — nothing is written.
}

} // namespace cfml
