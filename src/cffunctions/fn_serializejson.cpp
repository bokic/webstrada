/**
 * @file fn_serializejson.cpp
 * @brief CFML serializejson() built-in.
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

static json_object *serialize_json_from_args(const cfvariant *data,
    const cfvariant *queryFormatArg, const cfvariant *useSecureJSONPrefixArg,
    const cfvariant *useCustomSerializerArg)
{
    (void)useSecureJSONPrefixArg;
    (void)useCustomSerializerArg;
    string qf("row");
    if (queryFormatArg) {
        qf = const_cast<cfvariant*>(queryFormatArg)->toString();
    }
    g_serializeVisited.clear();
    return serialize_json_value(*data, qf, g_serializeVisited);
}

cfvariant *cf_serializejson(const cfvariant *data, const cfvariant *queryFormat,
                            const cfvariant *useSecureJSONPrefix,
                            const cfvariant *useCustomSerializer) {
    if (!data) throw webstrada::exception("SerializeJSON requires at least 1 argument");
    json_object *jsonObj = serialize_json_from_args(data, queryFormat, useSecureJSONPrefix, useCustomSerializer);
    const char *jsonStr = json_object_to_json_string_ext(jsonObj, JSON_C_TO_STRING_PLAIN | JSON_C_TO_STRING_NOSLASHESCAPE);
    auto *ret = new cfvariant(jsonStr ? jsonStr : "");
    json_object_put(jsonObj);
    return ret;
}

} // namespace cfml
