/**
 * @file fn_xmlparse.cpp
 * @brief CFML xmlparse() built-in.
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

cfvariant *cf_xmlparse(const cfvariant *arg0, const cfvariant *arg1, const cfvariant *arg2) {
    if (!arg0) throw webstrada::exception("XmlParse requires at least 1 argument");
    std::string xmlStr = safe_to_std_string(*arg0);
    bool caseSensitive = false;
    if (arg1) {
        caseSensitive = isTruthy(*arg1);
    }

    if (xmlStr.find("<") == std::string::npos && std::filesystem::exists(xmlStr)) {
        std::ifstream f(xmlStr);
        std::stringstream ss;
        ss << f.rdbuf();
        xmlStr = ss.str();
    }

    xmlDocPtr doc = xmlReadMemory(xmlStr.c_str(), xmlStr.length(), "noname.xml", nullptr, 0);
    if (!doc) {
        throw webstrada::exception("An error occurred while Parsing an XML document.");
    }

    cfvariant res = create_xml_document(doc, caseSensitive);
    xmlFreeDoc(doc);

    auto *ret = new cfvariant(res);
    return ret;
}

} // namespace cfml
