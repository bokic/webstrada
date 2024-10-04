/**
 * @file fn_xmlformat.cpp
 * @brief CFML xmlformat() built-in.
 */

#include "common.h"

#include "../cftags/common.h"
#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>
#include <webstrada/exceptions.h>
#include <webstrada/string.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <libxslt/xslt.h>
#include <libxslt/xsltInternals.h>
#include <libxslt/transform.h>
#include <libxslt/xsltutils.h>
#include <libxml/xmlschemas.h>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace cfml {

cfvariant *cf_xmlformat(const cfvariant *arg0, const cfvariant *arg1) {
    if (!arg0) throw webstrada::exception("XmlFormat requires at least 1 argument");
    std::string s = safe_to_std_string(*arg0);

    std::string res;
    for (char c : s) {
        if (c == '<') res += "&lt;";
        else if (c == '>') res += "&gt;";
        else if (c == '&') res += "&amp;";
        else if (c == '"') res += "&quot;";
        else if (c == '\'') res += "&apos;";
        else res += c;
    }
    auto *ret = new cfvariant(res.c_str());
    return ret;
}

} // namespace cfml
