/**
 * @file fn_deserializejson.cpp
 * @brief CFML deserializejson() built-in.
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

cfvariant *cf_deserializejson(const cfvariant *jsonArg, const cfvariant *strictMappingArg, bool literalBooleans) {
    if (!jsonArg) throw webstrada::exception("DeserializeJSON requires at least 1 argument");
    string jsonStr = const_cast<cfvariant*>(jsonArg)->toString();
    bool strictMapping = strictMappingArg ? cfvariant_is_truthy(strictMappingArg) : true;
    json_object *obj = json_tokener_parse(jsonStr.constData());
    if (!obj) {
        throw webstrada::exception("DeserializeJSON: Invalid JSON string");
    }
    cfvariant result = deserialize_json_value(obj, strictMapping, literalBooleans);
    json_object_put(obj);
    auto *ret = new cfvariant(result);
    return ret;
}

} // namespace cfml
