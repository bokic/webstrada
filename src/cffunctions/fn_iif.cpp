/**
 * @file fn_iif.cpp
 * @brief CFML iif() built-in.
 */

#include "common.h"

#include "../cftags/common.h"
#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>
#include <webstrada/exceptions.h>
#include <webstrada/string.h>
#include <string>

namespace cfml {

cfvariant *cf_iif(const cfvariant *condition, const cfvariant *expr1, const cfvariant *expr2,
                  string &out, void *cgi, void *server, void *cookie,
                  void *application, void *session, void *url, void *form,
                  void *variables) {
    if (!condition || !expr1 || !expr2) {
        throw webstrada::exception("Parameter validation error for the IIF function.");
    }
    bool cond = cf_is_truthy_value(condition);
    // The chosen expression is evaluated dynamically (like Evaluate), with the
    // same scopes as the calling template. The untaken branch is never
    // evaluated, matching CF (an invalid expression in the untaken branch does
    // not error).
    const cfvariant *arg = cond ? expr1 : expr2;
    return cf_evaluate(out, &arg, 1, cgi, server, cookie,
                       application, session, url, form, variables);
}

} // namespace cfml
