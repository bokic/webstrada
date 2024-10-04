/**
 * @file fn_serialize.cpp
 * @brief CFML serialize() built-in.
 */

#include "common.h"

#include "../cftags/common.h"
#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>
#include <webstrada/exceptions.h>
#include <webstrada/string.h>
#include <json-c/json.h>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <set>
#include <string>
#include <vector>

using webstrada::cfvariant;
using webstrada::string;
using webstrada::QueryData;

namespace cfml {

cfvariant *cf_serialize(const cfvariant *data, const cfvariant *type) {
    if (!data || !type) throw webstrada::exception("Serialize requires at least 2 arguments");
    string typeStr = const_cast<cfvariant*>(type)->toString();
    typeStr.toUpper();
    if (typeStr.equals("XML")) {
        throw webstrada::exception("Serialize: XML serialization is not yet supported");
    }
    if (!typeStr.equals("JSON")) {
        throw webstrada::exception(string("Serialize: Unsupported type '") + typeStr + "'. Supported types: json, xml");
    }
    return cf_serializejson(data, nullptr, nullptr, nullptr);
}

} // namespace cfml
