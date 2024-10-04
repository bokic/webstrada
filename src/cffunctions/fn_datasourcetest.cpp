/**
 * @file fn_datasourcetest.cpp
 * @brief Compiler-extension __datasourceTest() built-in.
 *
 * Attempts a real connection to a named <cfquery> datasource through the DB
 * layer and reports {verified: true} or {verified: false, error: <detail>}.
 * A datasource that is not configured reports {verified: false} with a
 * "not configured" message.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/config.h>
#include <webstrada/db.h>
#include <webstrada/exceptions.h>

#include <cctype>
#include <string>

namespace cfml {

static void putBool(cfvariant &st, const char *key, bool value)
{
    cfvariant v(cfvariant::Boolean);
    v.m_bool = value;
    st.structSet(key, v);
}

cfvariant *cf___datasourcetest(const cfvariant **args, int argc)
{
    cfvariant result(cfvariant::Struct);

    if (argc < 1 || !args || !args[0]) {
        putBool(result, "verified", false);
        result.structSet("error", cfvariant("__datasourceTest requires a datasource name argument."));
        return new cfvariant(result);
    }

    const cfvariant *arg = args[0];
    if (arg->m_type == cfvariant::Struct && arg->m_struct->find(CFML_NAMED_ARGS_KEY) != arg->m_struct->end()) {
        putBool(result, "verified", false);
        result.structSet("error", cfvariant("__datasourceTest does not accept named arguments."));
        return new cfvariant(result);
    }

    std::string name = toStdString(arg);
    std::string nameUp = name;
    for (auto &c : nameUp) c = static_cast<char>(toupper((unsigned char)c));

    auto dsIt = webstrada::config::datasources.find(nameUp);
    if (dsIt == webstrada::config::datasources.end()) {
        putBool(result, "verified", false);
        result.structSet("error", cfvariant(("Datasource '" + name + "' is not configured.").c_str()));
        return new cfvariant(result);
    }

    try {
        webstrada::db::DBConnection *conn = webstrada::db::openConnection(name, 5000);
        if (!conn) {
            putBool(result, "verified", false);
            result.structSet("error", cfvariant("Connection failed (no connection object)."));
            return new cfvariant(result);
        }
        delete conn;
        putBool(result, "verified", true);
    } catch (const webstrada::exception &ex) {
        putBool(result, "verified", false);
        std::string detail = ex.m_message.isEmpty() ? "" : ex.m_message.constData();
        if (!ex.m_detail.isEmpty()) {
            if (!detail.empty()) detail += " ";
            detail += ex.m_detail.constData();
        }
        if (detail.empty()) detail = "Connection failed.";
        result.structSet("error", cfvariant(detail.c_str()));
    } catch (const std::exception &ex) {
        putBool(result, "verified", false);
        result.structSet("error", cfvariant(ex.what()));
    }

    return new cfvariant(result);
}

} // namespace cfml
