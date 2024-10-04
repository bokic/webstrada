/**
 * @file fn_createodbcdate.cpp
 * @brief CFML createodbcdate() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <chrono>
#include <ctime>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <map>
#include <string>

namespace cfml {

cfvariant *cf_createodbcdate(const cfvariant *date) {
    if (!date) throw webstrada::exception("CreateODBCDate requires exactly 1 argument");
    double days = 0.0;
    if (!getDaysFromVariant(date, days)) {
        string val = const_cast<cfvariant*>(date)->toString();
        if (looksLikeDateString(val)) {
            webstrada::string msg = val;
            msg.append(" is an invalid date or time string.");
            throw webstrada::exception(msg);
        }
        throw webstrada::exception("The value " + val + " cannot be converted to a date.");
    }
    cfvariant res(cfvariant::DateTime);
    res.m_double = days;
    res.m_odbcStyle = 1;
    auto *ret = new cfvariant(res);
    return ret;
}

} // namespace cfml
