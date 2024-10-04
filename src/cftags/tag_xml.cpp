/**
 * @file tag_xml.cpp
 * @brief <cfxml> tag runtime implementations.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>
#include <webstrada/exceptions.h>
#include <webstrada/string.h>

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <cstddef>
#include <string>

namespace cfml {

webstrada::string *cf_xml_begin()
{
    return silent_buf_push();
}

void cf_xml_end(string *out, void *cgi, void *server, void *cookie, void *application,
                      void *session, void *url, void *form, void *variables,
                      const cfvariant *varName, const cfvariant *caseSensitive)
{
    std::string body;
    if (out) {
        const char *d = out->constData();
        if (d) body.assign(d, out->length());
    }
    silent_buf_pop();

    size_t b = 0, e = body.size();
    while (b < e && static_cast<unsigned char>(body[b]) <= 0x20) b++;
    while (e > b && static_cast<unsigned char>(body[e - 1]) <= 0x20) e--;
    std::string trimmed = body.substr(b, e - b);

    bool caseSens = caseSensitive ? cfvariant_is_truthy(caseSensitive) : false;

    xmlDocPtr doc = xmlReadMemory(trimmed.c_str(), trimmed.length(), "noname.xml", nullptr, 0);
    if (!doc) {
        throw webstrada::exception("An error occurred while Parsing an XML document.");
    }
    cfvariant res = create_xml_document(doc, caseSens);
    xmlFreeDoc(doc);

    std::string name = safe_to_std_string(*varName);
    cfvariant_assign(static_cast<const cfvariant*>(cgi), static_cast<const cfvariant*>(server),
                     static_cast<const cfvariant*>(cookie), static_cast<const cfvariant*>(application),
                     static_cast<const cfvariant*>(session), static_cast<const cfvariant*>(url),
                     static_cast<const cfvariant*>(form), static_cast<cfvariant*>(variables),
                     name.c_str(), &res);
}

} // namespace cfml
