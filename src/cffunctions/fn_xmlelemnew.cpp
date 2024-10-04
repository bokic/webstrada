/**
 * @file fn_xmlelemnew.cpp
 * @brief CFML xmlelemnew() built-in.
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

cfvariant *cf_xmlelemnew(const cfvariant *arg0, const cfvariant *arg1, const cfvariant *arg2) {
    if (!arg0 || !arg1) throw webstrada::exception("XmlElemNew requires at least 2 arguments");
    std::string childName;
    std::string ns;
    if (arg2) {
        ns = safe_to_std_string(*arg1);
        childName = safe_to_std_string(*arg2);
    } else {
        childName = safe_to_std_string(*arg1);
    }

    cfvariant elem(arg0->m_upcase, false);
    elem.set_type(cfvariant::Xml);
    elem.set("XMLNAME") = cfvariant(childName.c_str());
    elem.set("XMLTYPE") = cfvariant("ELEMENT");
    elem.set("XMLVALUE") = cfvariant("");
    elem.set("XMLTEXT") = cfvariant("");

    cfvariant attrs(arg0->m_upcase, false);
    attrs.set_type(cfvariant::Struct);
    elem.set("XMLATTRIBUTES") = attrs;

    cfvariant children(arg0->m_upcase, false);
    children.set_type(cfvariant::Array);
    elem.set("XMLCHILDREN") = children;

    cfvariant nodes(arg0->m_upcase, false);
    nodes.set_type(cfvariant::Array);
    elem.set("XMLNODES") = nodes;

    auto *ret = new cfvariant(elem);
    return ret;
}

} // namespace cfml
