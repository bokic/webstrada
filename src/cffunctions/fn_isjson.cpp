/**
 * @file fn_isjson.cpp
 * @brief CFML isjson() built-in.
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

cfvariant *cf_isjson(const cfvariant *arg) {
    if (!arg) return cfvariant_create_bool(false);
    string jsonStr = const_cast<cfvariant*>(arg)->toString();
    if (jsonStr.length() == 0) return cfvariant_create_bool(false);
    bool valid = false;
    json_object *obj = json_tokener_parse(jsonStr.constData());
    if (obj != nullptr) {
        valid = true;
        json_object_put(obj);
    }
    return cfvariant_create_bool(valid);
}

} // namespace cfml
