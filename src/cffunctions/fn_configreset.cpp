/**
 * @file fn_configreset.cpp
 * @brief Compiler-extension __configReset() built-in.
 *
 * Resets every server-wide config::* global to the built-in defaults, persists
 * them to the config file, and returns __configGet()'s updated configuration.
 * Backs the admin panel's "Restore Defaults" action.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/config.h>

namespace cfml {

cfvariant *cf___configget(const cfvariant **args, int argc);

cfvariant *cf___configreset(const cfvariant **args, int argc)
{
    (void)args;
    (void)argc;
    webstrada::config::resetToDefaults();
    return cf___configget(nullptr, 0);
}

} // namespace cfml
