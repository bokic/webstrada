/**
 * @file fn_ajax.cpp
 * @brief CFML ajaxLink() / ajaxOnLoad() / invokeCFClientFunction() built-ins.
 *
 * ajaxLink(url): CF 2025 returns the URL unchanged when the request has no Ajax
 * container (HtmlAssembler.getContainerId() == null), which is the normal case
 * for a plain page - this engine never creates Ajax containers, so the URL is
 * always returned as-is (byte-verified on the RDS host).
 *
 * ajaxOnLoad(functionName): emits an inline script that registers the function
 * for the window onload event. CF output (byte-verified):
 *   newline, script-open, CDATA-open, tab,
 *   ColdFusion.Event.registerOnLoad(&lt;fn&gt;,null,false,true);,
 *   newline, CDATA-close, script-close, newline
 * The function value is emitted verbatim (no quoting/escaping).
 *
 * invokeCFClientFunction(): NOT a ColdFusion 2025 function - CF reports
 * "Variable INVOKECFCLIENTFUNCTION is undefined." (byte-verified on the RDS
 * host). The stub reproduces that error.
 */

#include "common.h"

#include "../cftags/common.h"
#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>
#include <webstrada/exceptions.h>
#include <webstrada/string.h>

namespace cfml {

using webstrada::cfvariant;
using webstrada::string;

cfvariant *cf_ajaxlink(const cfvariant *url)
{
    if (!url) throw webstrada::exception("ajaxLink requires at least 1 argument");
    // No Ajax container exists in this engine, so the URL is returned unchanged
    // (CF's behavior when getContainerId() == null).
    auto *ret = new cfvariant(*url);
    cf_register_temp(ret);
    return ret;
}

cfvariant *cf_ajaxonload(const cfvariant *functionName, string &out)
{
    if (!functionName) throw webstrada::exception("ajaxOnLoad requires at least 1 argument");
    string fn = const_cast<cfvariant*>(functionName)->toString();

    // Reproduce CF's HtmlAssembler script block verbatim. It goes to the
    // response <head>, which CF emits before the body output regardless of
    // where in the page it was produced.
    string js;
    js.append("\n<script type=\"text/javascript\" >/* <![CDATA[ */\n\t");
    js.append("ColdFusion.Event.registerOnLoad(");
    js.append(fn);
    js.append(",null,false,true);\n/* ]]> */</script>\n");
    (void)out;
    cfml::response().headContent.append(js);

    auto *ret = new cfvariant(cfvariant::Null);
    cf_register_temp(ret);
    return ret;
}

cfvariant *cf_invokecfclientfunction(const cfvariant *arg)
{
    (void)arg;
    throw webstrada::exception("Variable INVOKECFCLIENTFUNCTION is undefined.");
}

} // namespace cfml
