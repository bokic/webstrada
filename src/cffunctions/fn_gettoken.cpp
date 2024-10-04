/**
 * @file fn_gettoken.cpp
 * @brief CFML gettoken() built-in.
 */

#include "common.h"

#include "../cftags/common.h"
#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>
#include <webstrada/exceptions.h>
#include <webstrada/string.h>
#include <string>
#include <vector>

namespace cfml {

cfvariant *cf_gettoken(const cfvariant *str, const cfvariant *index, const cfvariant *delimiters) {
    if (!str || !index) throw webstrada::exception("GetToken requires at least 2 arguments");
    // Java StringTokenizer semantics: every character of the delimiter string
    // separates tokens; consecutive delimiters collapse and empty tokens are
    // never returned. The default delimiter set is " \t\r\n" (CR, LF, tab,
    // space) — NOT a comma (verified against CF 2025 on the RDS host; the
    // decompiled CFPage.GetToken defaults to "\r\n\t ").
    webstrada::string delim;
    if (delimiters) {
        delim = const_cast<cfvariant*>(delimiters)->toString();
    } else {
        delim = "\r\n\t ";
    }

    int idx = cfvariant_to_int(index);
    if (idx <= 0) {
        // CF: "The value of parameter 2 of the function GetToken, which is now
        // N, must be a positive integer" (verified against CF 2025).
        webstrada::string msg("The value of parameter 2 of the function GetToken, which is now ");
        msg.append(webstrada::string::number(idx));
        msg.append(", must be a positive integer");
        throw webstrada::exception(msg);
    }

    webstrada::string s = const_cast<cfvariant*>(str)->toString();
    std::vector<webstrada::string> tokens;
    if (!s.isEmpty()) {
        webstrada::string current;
        bool inToken = false;
        for (int i = 0; i < s.length(); i++) {
            char c = s.at(i);
            bool isSep = false;
            for (int d = 0; d < delim.length(); d++) {
                if (c == delim.at(d)) { isSep = true; break; }
            }
            if (isSep) {
                if (inToken) {
                    tokens.push_back(current);
                    current.clear();
                    inToken = false;
                }
            } else {
                current.append(c);
                inToken = true;
            }
        }
        if (inToken) tokens.push_back(current);
    }

    if ((int)tokens.size() < idx) {
        return new cfvariant("");
    }
    return new cfvariant(tokens[idx - 1]);
}

} // namespace cfml
