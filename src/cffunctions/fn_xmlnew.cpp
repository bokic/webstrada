/**
 * @file fn_xmlnew.cpp
 * @brief CFML xmlnew() built-in.
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

cfvariant *cf_xmlnew(const cfvariant *arg0) {
    bool caseSensitive = false;
    if (arg0) {
        caseSensitive = isTruthy(*arg0);
    }
    cfvariant doc(caseSensitive, false);
    doc.set_type(cfvariant::Xml);
    doc.set("XMLTYPE") = cfvariant("DOCUMENT");

    auto *ret = new cfvariant(doc);
    return ret;
}

} // namespace cfml
