/**
 * @file fn_canserialize.cpp
 * @brief CFML canserialize() built-in.
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

cfvariant *cf_canserialize() {
    auto *ret = new cfvariant(cfvariant::Boolean);
    ret->m_bool = true;
    return ret;
}

} // namespace cfml
