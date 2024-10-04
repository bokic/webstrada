/**
 * @file fn_csv.cpp
 * @brief CFML csvread() / csvwrite() / csvprocess() built-ins.
 *
 * CF 2025 routes these to the spreadsheet (Excel) service which is not
 * installed on the RDS host (throws "The spreadsheet package is not
 * installed."), so the exact CF output cannot be byte-verified. The
 * implementations below follow the documented RFC-4180 behavior from
 * current_progress: CSVRead parses data (or a file path) into a query,
 * CSVWrite serializes an array/query into CSV, CSVProcess runs a callback per
 * row.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>
#include <webstrada/exceptions.h>
#include <webstrada/string.h>
#include <cctype>
#include <fstream>
#include <string>
#include <vector>

namespace cfml {

namespace {

// RFC-4180 CSV parser. Returns rows of fields.
std::vector<std::vector<std::string>> parseCsv(const std::string &data, char delim) {
    std::vector<std::vector<std::string>> rows;
    std::vector<std::string> row;
    std::string field;
    bool inQuotes = false;
    size_t i = 0;
    const size_t n = data.size();
    while (i < n) {
        char c = data[i];
        if (inQuotes) {
            if (c == '"') {
                if (i + 1 < n && data[i + 1] == '"') {
                    field += '"';
                    i += 2;
                } else {
                    inQuotes = false;
                    i++;
                }
            } else {
                field += c;
                i++;
            }
        } else {
            if (c == '"') {
                inQuotes = true;
                i++;
            } else if (c == delim) {
                row.push_back(field);
                field.clear();
                i++;
            } else if (c == '\n' || c == '\r') {
                if (c == '\r' && i + 1 < n && data[i + 1] == '\n') i++;
                row.push_back(field);
                field.clear();
                rows.push_back(row);
                row.clear();
                i++;
            } else {
                field += c;
                i++;
            }
        }
    }
    if (!field.empty() || !row.empty() || inQuotes) {
        row.push_back(field);
        rows.push_back(row);
    }
    // Drop a trailing empty row (e.g. data ending in a newline).
    if (!rows.empty() && rows.back().size() == 1 && rows.back()[0].empty()) {
        rows.pop_back();
    }
    return rows;
}

// RFC-4180 CSV writer: quote a field when it contains the delimiter, a quote,
// a newline or CR.
std::string quoteCsvField(const std::string &f, char delim) {
    bool needsQuote = f.find(delim) != std::string::npos ||
                      f.find('"') != std::string::npos ||
                      f.find('\n') != std::string::npos ||
                      f.find('\r') != std::string::npos;
    if (!needsQuote) return f;
    std::string out = "\"";
    for (char c : f) {
        if (c == '"') out += "\"\"";
        else out += c;
    }
    out += "\"";
    return out;
}

std::string csvFieldToString(const cfvariant &v) {
    if (v.m_type == cfvariant::Null) return "";
    return const_cast<cfvariant&>(v).toString().constData();
}

} // namespace

cfvariant *cf_csvread(const cfvariant *data, const cfvariant *columns,
                      const cfvariant *delimiter, const cfvariant *charset) {
    (void)charset;
    if (!data) throw webstrada::exception("CSVRead requires at least 1 argument");
    bool firstIsHeader = !(columns && cf_is_truthy_value(columns));
    char delim = ',';
    if (delimiter) {
        webstrada::string d = const_cast<cfvariant*>(delimiter)->toString();
        if (!d.isEmpty()) delim = d.at(0);
    }
    webstrada::string input = const_cast<cfvariant*>(data)->toString();
    std::string csv = input.constData() ? input.constData() : "";
    // Accept either CSV data or a file path.
    if (csv.size() > 0 && csv.find('\n') == std::string::npos &&
        csv.find(',') == std::string::npos && csv.find(';') == std::string::npos &&
        csv.find('"') == std::string::npos) {
        std::ifstream in(csv, std::ios::binary);
        if (in) {
            csv.assign((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        }
    }
    auto rows = parseCsv(csv, delim);

    cfvariant queryVal(cfvariant::Query);
    QueryData *qd = queryVal.m_query;
    size_t colCount = 0;
    for (const auto &r : rows) if (r.size() > colCount) colCount = r.size();
    size_t startRow = (firstIsHeader && !rows.empty()) ? 1 : 0;

    for (size_t c = 0; c < colCount; c++) {
        QueryColumn col;
        if (firstIsHeader && !rows.empty() && c < rows[0].size() && !rows[0][c].empty()) {
            col.name = rows[0][c].c_str();
        } else {
            col.name = webstrada::string(("column" + std::to_string(c + 1)).c_str());
        }
        col.type = "varchar";
        qd->columns.push_back(col);
    }

    for (size_t r = startRow; r < rows.size(); r++) {
        for (size_t c = 0; c < colCount; c++) {
            std::string val = (c < rows[r].size()) ? rows[r][c] : "";
            qd->columns[c].values.push_back(cfvariant(val.c_str()));
        }
        qd->m_rowCount++;
    }

    auto *ret = new cfvariant(queryVal);
    return ret;
}

cfvariant *cf_csvwrite(const cfvariant *data, const cfvariant *delimiter) {
    if (!data) throw webstrada::exception("CSVWrite requires at least 1 argument");
    char delim = ',';
    if (delimiter) {
        webstrada::string d = const_cast<cfvariant*>(delimiter)->toString();
        if (!d.isEmpty()) delim = d.at(0);
    }
    std::string out;

    if (data->m_type == cfvariant::Query && data->m_query) {
        QueryData *qd = data->m_query;
        for (size_t c = 0; c < qd->columns.size(); c++) {
            if (c > 0) out += delim;
            out += quoteCsvField(qd->columns[c].name.constData(), delim);
        }
        out += "\n";
        for (int r = 0; r < qd->m_rowCount; r++) {
            for (size_t c = 0; c < qd->columns.size(); c++) {
                if (c > 0) out += delim;
                std::string v;
                if (r < (int)qd->columns[c].values.size()) {
                    v = csvFieldToString(qd->columns[c].values[r]);
                }
                out += quoteCsvField(v, delim);
            }
            out += "\n";
        }
    } else if (data->m_type == cfvariant::Array && data->m_array) {
        for (const auto &rowVal : *data->m_array) {
            if (rowVal.m_type == cfvariant::Array) {
                size_t cc = 0;
                for (const auto &cell : *rowVal.m_array) {
                    if (cc > 0) out += delim;
                    out += quoteCsvField(csvFieldToString(cell), delim);
                    cc++;
                }
                out += "\n";
            } else {
                out += quoteCsvField(csvFieldToString(rowVal), delim);
                out += "\n";
            }
        }
    }
    return new cfvariant(out.c_str());
}

cfvariant *cf_csvprocess(const cfvariant *data, const cfvariant *callback,
                         const cfvariant *delimiter, string &out,
                         void *cgi, void *server, void *cookie,
                         void *application, void *session, void *url,
                         void *form, void *variables) {
    if (!data || !callback) throw webstrada::exception("CSVProcess requires at least 2 arguments");
    char delim = ',';
    if (delimiter) {
        webstrada::string d = const_cast<cfvariant*>(delimiter)->toString();
        if (!d.isEmpty()) delim = d.at(0);
    }
    webstrada::string input = const_cast<cfvariant*>(data)->toString();
    std::string csv = input.constData() ? input.constData() : "";
    auto rows = parseCsv(csv, delim);

    cfvariant cb = *const_cast<cfvariant*>(callback);
    for (const auto &row : rows) {
        cfvariant rowArr(cfvariant::Array);
        for (const auto &f : row) {
            rowArr.insert(cfvariant(f.c_str()));
        }
        std::vector<cfvariant> cbArgs = { rowArr };
        callCallback(out, cb, cbArgs, cgi, server, cookie, application, session, url, form, variables);
    }
    return nullResult();
}

} // namespace cfml
