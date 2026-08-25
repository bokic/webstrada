/**
 * @file db_qoq.cpp
 * @brief Query of Queries (QoQ) engine backed by an in-memory SQLite database.
 *
 * ColdFusion's `dbtype="query"` runs a SQL statement against in-memory query
 * objects instead of a database. The engine materializes each referenced query
 * object as a temporary table in a private `:memory:` SQLite database and hands
 * the statement to SQLite's own SQL engine, which buys the full SELECT dialect
 * (joins, aggregates, GROUP BY/ORDER BY, DISTINCT, subqueries, ...) for free.
 *
 * Table materialization is lazy and driven by SQLite itself: a prepare fails
 * with "no such table: X", the name X is resolved through the caller's
 * `QoQResolver` callback (a CFML scope lookup), the query object's columns and
 * typed cells are copied into a table, and the prepare is retried. This handles
 * every reference form (FROM lists, JOINs, subqueries, UPDATE/DELETE FROM)
 * without re-implementing SQL parsing.
 *
 * Two dialect bridges reproduce CFQL semantics on SQLite:
 *   * `+` string concatenation -- HSQLDB (CF's QoQ engine) concatenates when a
 *     `+` operand is a string literal; SQLite coerces to numbers. A `+` with a
 *     string-literal operand is rewritten to SQLite's `||`.
 *   * Date cells are stored as ISO text 'YYYY-MM-DD HH:MM:SS' so the same
 *     literals <cfqueryparam> emits compare correctly.
 *
 * Only SELECT statements are supported: INSERT/UPDATE/DELETE would have to
 * write changes back into the source query objects, which is not implemented
 * (they throw "not implemented" rather than silently mutating throwaway
 * copies).
 */

#include <webstrada/db.h>
#include <webstrada/exceptions.h>
#include <webstrada/config.h>
#include <webstrada/cf8.h>

#include <sqlite3.h>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <set>
#include <string>
#include <vector>

namespace webstrada {
namespace db {

namespace {

// ---- small helpers --------------------------------------------------------

std::string toStd(const webstrada::string &s)
{
    return s.constData() ? std::string(s.constData()) : std::string();
}

std::string trimStr(const std::string &s)
{
    size_t b = 0, e = s.size();
    while (b < e && static_cast<unsigned char>(s[b]) <= 0x20) b++;
    while (e > b && static_cast<unsigned char>(s[e - 1]) <= 0x20) e--;
    return s.substr(b, e - b);
}

std::string ciKey(const std::string &s)
{
    std::string out = s;
    for (char &c : out) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
    return out;
}

// Double-quote an identifier ("" escapes an embedded quote). SQLite matches
// quoted identifiers case-insensitively, so a source query named `myQuery`
// resolves against a `FROM MYQUERY` reference.
std::string quoteIdent(const std::string &name)
{
    std::string out = "\"";
    for (char c : name) {
        if (c == '"') out += "\"\"";
        else out += c;
    }
    out += "\"";
    return out;
}

[[noreturn]] void throwDatabaseError(const std::string &detail)
{
    throw webstrada::exception("Database", "Error Executing Database Query.",
                              detail.c_str());
}

void execOrThrow(sqlite3 *db, const std::string &sql)
{
    char *err = nullptr;
    int rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &err);
    if (rc != SQLITE_OK) {
        std::string msg = err ? err : sqlite3_errmsg(db);
        sqlite3_free(err);
        throwDatabaseError(msg);
    }
}

// ---- CFQL string-concatenation rewrite ------------------------------------

enum TokKind { TkWs, TkComment, TkString, TkQIdent, TkNumber, TkWord, TkOp };

struct Tok { TokKind kind; size_t start, end; };

// Tokenize SQL, tracking string literals, quoted identifiers, numbers (with
// exponents, so `1.5e+3` is one token) and comments. Everything else is a
// word or a single-character operator token.
std::vector<Tok> tokenize(const std::string &s)
{
    std::vector<Tok> toks;
    size_t i = 0, n = s.size();
    auto push = [&](TokKind k, size_t a, size_t b) { toks.push_back({k, a, b}); };
    while (i < n) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (isspace(c)) {
            size_t a = i;
            while (i < n && isspace(static_cast<unsigned char>(s[i]))) i++;
            push(TkWs, a, i);
        } else if (c == '-' && i + 1 < n && s[i + 1] == '-') {
            size_t a = i;
            i += 2;
            while (i < n && s[i] != '\n') i++;
            push(TkComment, a, i);
        } else if (c == '/' && i + 1 < n && s[i + 1] == '*') {
            size_t a = i;
            i += 2;
            while (i + 1 < n && !(s[i] == '*' && s[i + 1] == '/')) i++;
            i = std::min(i + 2, n);
            push(TkComment, a, i);
        } else if (c == '\'') {
            size_t a = i;
            i++;
            while (i < n) {
                if (s[i] == '\'') {
                    if (i + 1 < n && s[i + 1] == '\'') i += 2;
                    else { i++; break; }
                } else i++;
            }
            push(TkString, a, i);
        } else if (c == '"' || c == '[' || c == '`') {
            char close = (c == '[') ? ']' : c;
            size_t a = i;
            i++;
            while (i < n) {
                if (s[i] == close) {
                    if (close == '"' && i + 1 < n && s[i + 1] == '"') { i += 2; continue; }
                    i++;
                    break;
                }
                i++;
            }
            push(TkQIdent, a, i);
        } else if (isdigit(c) || (c == '.' && i + 1 < n && isdigit(static_cast<unsigned char>(s[i + 1])))) {
            size_t a = i;
            while (i < n && (isdigit(static_cast<unsigned char>(s[i])) || s[i] == '.')) i++;
            if (i < n && (s[i] == 'e' || s[i] == 'E')) {
                size_t j = i + 1;
                if (j < n && (s[j] == '+' || s[j] == '-')) j++;
                if (j < n && isdigit(static_cast<unsigned char>(s[j]))) {
                    while (j < n && isdigit(static_cast<unsigned char>(s[j]))) j++;
                    i = j;
                }
            }
            push(TkNumber, a, i);
        } else if (isalpha(c) || c == '_' || c == '$' || c >= 0x80) {
            size_t a = i;
            while (i < n) {
                unsigned char d = static_cast<unsigned char>(s[i]);
                if (isalnum(d) || d == '_' || d == '$' || d >= 0x80) i++;
                else break;
            }
            push(TkWord, a, i);
        } else {
            push(TkOp, i, i + 1);
            i++;
        }
    }
    return toks;
}

// Materialized-table schema: ci(table) -> ci(column) -> affinity ("TEXT",
// "INTEGER", "REAL" or "BLOB"). Used to classify `+` operands as strings vs
// numbers so the CFQL concatenation rule can be reproduced (see
// rewriteConcatOperators).
typedef std::map<std::string, std::map<std::string, std::string>> SchemaMap;

// CFQL (HSQLDB, CF's QoQ engine) `+` semantics differ from SQLite's numeric
// `+`. Verified against CF 2025 on the RDS host:
//   * string + string (string literals and/or TEXT columns)   -> concatenates
//   * string + number (literal or numeric column)             -> Database error
//   * number + number                                         -> numeric add
// The rewrite turns every string+string `+` into SQLite's `||` and throws CF's
// "Error Executing Database Query." for string+number, which SQLite would
// silently coerce. Numeric and unresolvable-operand `+` are left to SQLite.
std::string rewriteConcatOperators(const std::string &sql, const SchemaMap &schemas)
{
    std::vector<Tok> toks = tokenize(sql);
    std::vector<size_t> sig;
    for (size_t i = 0; i < toks.size(); i++) {
        if (toks[i].kind != TkWs && toks[i].kind != TkComment) sig.push_back(i);
    }

    // Classify the operand at significant-token index k: 'S' (string literal or
    // TEXT column), 'N' (numeric literal or numeric column), 'U' (unresolvable:
    // functions, parenthesized expressions, unknown columns).
    auto operandKind = [&](size_t k) -> char {
        if (k >= sig.size()) return 'U';
        const Tok &t = toks[sig[k]];
        if (t.kind == TkString) return 'S';
        if (t.kind == TkNumber) return 'N';
        if (t.kind == TkWord) {
            std::string word = sql.substr(t.start, t.end - t.start);
            std::string qualifier;
            // `table.col`: the previous significant token is '.' with a WORD
            // before it. Qualifier resolution is skipped when the qualifier is
            // not a materialized table (e.g. a subquery alias) -> 'U'.
            if (k >= 2 && toks[sig[k - 1]].kind == TkOp && sql[toks[sig[k - 1]].start] == '.' &&
                toks[sig[k - 2]].kind == TkWord) {
                qualifier = sql.substr(toks[sig[k - 2]].start, toks[sig[k - 2]].end - toks[sig[k - 2]].start);
            }
            std::string ck = ciKey(word);
            int text = 0, num = 0;
            for (const auto &[tname, cols] : schemas) {
                if (!qualifier.empty() && ciKey(qualifier) != tname) continue;
                auto it = cols.find(ck);
                if (it == cols.end()) continue;
                if (it->second == "TEXT") text++;
                else if (it->second == "INTEGER" || it->second == "REAL") num++;
            }
            if (text && !num) return 'S';
            if (num && !text) return 'N';
        }
        return 'U';
    };

    std::string out;
    size_t pos = 0;
    for (size_t k = 0; k < sig.size(); k++) {
        const Tok &t = toks[sig[k]];
        out.append(sql, pos, t.start - pos);
        if (t.kind == TkOp && t.end - t.start == 1 && sql[t.start] == '+') {
            char left = operandKind(k - 1);
            char right = operandKind(k + 1);
            if (left == 'S' && right == 'S') {
                out += "||";
            } else if ((left == 'S' && right == 'N') || (left == 'N' && right == 'S')) {
                // CF throws a Database error for string + number (SQLite would
                // silently coerce both to numbers).
                throwDatabaseError("Query Of Queries runtime error.");
            } else {
                out += "+";
            }
        } else {
            out.append(sql, t.start, t.end - t.start);
        }
        pos = t.end;
    }
    out.append(sql, pos, sql.size() - pos);
    return out;
}

// ---- table materialization ------------------------------------------------

// SQLite declared type that gives a column the affinity closest to CF's
// per-cell Java types (so `col = '25'` / `col = 25` compare like HSQLDB).
std::string columnDeclType(const QueryColumn &col)
{
    bool f = false, d = false, i = false, b = false, t = false;
    for (const auto &v : col.values) {
        switch (v.m_type) {
        case cfvariant::Float: f = true; break;
        case cfvariant::DateTime: d = true; break;
        case cfvariant::Number:
        case cfvariant::Long:
        case cfvariant::Boolean: i = true; break;
        case cfvariant::Binary: b = true; break;
        case cfvariant::String: t = true; break;
        default: break;
        }
    }
    if (f) return "REAL";
    if (d) return "TEXT";
    if (i && !t && !b) return "INTEGER";
    if (b && !i && !t && !f && !d) return "BLOB";
    return "TEXT";
}

void bindCell(sqlite3_stmt *stmt, int idx, const cfvariant &v)
{
    switch (v.m_type) {
    case cfvariant::Null:
        sqlite3_bind_null(stmt, idx);
        break;
    case cfvariant::Boolean:
        sqlite3_bind_int(stmt, idx, v.m_bool ? 1 : 0);
        break;
    case cfvariant::Number:
        sqlite3_bind_int(stmt, idx, v.m_int);
        break;
    case cfvariant::Long:
        sqlite3_bind_int64(stmt, idx, v.m_long);
        break;
    case cfvariant::Float:
        sqlite3_bind_double(stmt, idx, v.m_double);
        break;
    case cfvariant::DateTime: {
        struct tm tmv = cfml::daysToTm(v.m_double);
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d",
                      tmv.tm_year + 1900, tmv.tm_mon + 1, tmv.tm_mday,
                      tmv.tm_hour, tmv.tm_min, tmv.tm_sec);
        sqlite3_bind_text(stmt, idx, buf, -1, SQLITE_TRANSIENT);
        break;
    }
    case cfvariant::Binary:
        if (v.m_binary && !v.m_binary->empty()) {
            sqlite3_bind_blob(stmt, idx, static_cast<const void*>(v.m_binary->data()),
                              static_cast<int>(v.m_binary->size()), SQLITE_TRANSIENT);
        } else {
            sqlite3_bind_null(stmt, idx);
        }
        break;
    case cfvariant::String:
        if (v.m_str && v.m_str->constData()) {
            sqlite3_bind_text(stmt, idx, v.m_str->constData(), -1, SQLITE_TRANSIENT);
        } else {
            sqlite3_bind_null(stmt, idx);
        }
        break;
    default: {
        // Composite cell values (struct/array/query/...) degrade to their text
        // form, mirroring CF's QoQ stringification of non-scalar cells.
        webstrada::string txt = const_cast<cfvariant&>(v).toString();
        if (txt.constData()) sqlite3_bind_text(stmt, idx, txt.constData(), -1, SQLITE_TRANSIENT);
        else sqlite3_bind_text(stmt, idx, "", 0, SQLITE_STATIC);
        break;
    }
    }
}

// Create `name` as a temporary table holding `qd`'s columns and rows, and
// record its column affinities in `schemas` (used by the `+` concat rewrite).
void createTableFromQuery(sqlite3 *db, const std::string &name, QueryData *qd,
                          SchemaMap &schemas)
{
    std::string qn = quoteIdent(name);
    std::string cols;
    auto &tbl = schemas[ciKey(name)];
    for (size_t c = 0; c < qd->columns.size(); c++) {
        if (c) cols += ", ";
        cols += quoteIdent(toStd(qd->columns[c].name));
        cols += " ";
        std::string decl = columnDeclType(qd->columns[c]);
        cols += decl;
        tbl[ciKey(toStd(qd->columns[c].name))] = decl;
    }
    if (cols.empty()) cols = "_ws_qoq_dummy INTEGER";
    execOrThrow(db, "CREATE TABLE " + qn + " (" + cols + ")");

    int nCols = static_cast<int>(qd->columns.size());
    if (nCols == 0) return;
    std::string ph;
    for (int c = 0; c < nCols; c++) { if (c) ph += ","; ph += "?"; }
    std::string ins = "INSERT INTO " + qn + " VALUES (" + ph + ")";

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, ins.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throwDatabaseError(sqlite3_errmsg(db));
    }
    for (int r = 0; r < qd->rowCount(); r++) {
        for (int c = 0; c < nCols; c++) {
            if (static_cast<size_t>(r) < qd->columns[c].values.size()) {
                bindCell(stmt, c + 1, qd->columns[c].values[r]);
            } else {
                sqlite3_bind_null(stmt, c + 1);
            }
        }
        if (sqlite3_step(stmt) != SQLITE_DONE) {
            std::string msg = sqlite3_errmsg(db);
            sqlite3_finalize(stmt);
            throwDatabaseError(msg);
        }
        sqlite3_reset(stmt);
        sqlite3_clear_bindings(stmt);
    }
    sqlite3_finalize(stmt);
}

// "no such table: main.foo" / "no such table: foo" -> "foo". Returns "" when
// the error is not a missing-table error.
std::string extractMissingTableName(const char *err)
{
    if (!err) return "";
    std::string e(err);
    std::string lower = e;
    for (char &c : lower) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
    const char *marker = "no such table: ";
    size_t pos = lower.find(marker);
    if (pos == std::string::npos) return "";
    std::string name = e.substr(pos + std::strlen(marker));
    name = trimStr(name);
    if (name.rfind("main.", 0) == 0) name = name.substr(5);
    else if (name.rfind("temp.", 0) == 0) name = name.substr(5);
    return name;
}

// Prepare the next statement from *pzTail, materializing a missing table the
// first time its name is seen. Returns the statement, or nullptr when only
// whitespace/comments remain. Throws for any non-recoverable prepare error.
sqlite3_stmt *prepareAndMaterialize(sqlite3 *db, const char *&pzTail,
                                    std::set<std::string> &created,
                                    SchemaMap &schemas,
                                    const QoQResolver &resolver)
{
    for (int attempt = 0; attempt < 1000; attempt++) {
        sqlite3_stmt *stmt = nullptr;
        const char *next = nullptr;
        int rc = sqlite3_prepare_v2(db, pzTail, -1, &stmt, &next);
        if (rc == SQLITE_OK) {
            pzTail = next;
            return stmt;
        }
        const char *err = sqlite3_errmsg(db);
        std::string table = extractMissingTableName(err);
        if (table.empty() || !created.insert(ciKey(table)).second) {
            throwDatabaseError(err);
        }
        const cfvariant *qv = resolver(table);  // throws for undefined/non-query
        if (!qv || qv->m_type != cfvariant::Query || !qv->m_query) {
            throwDatabaseError(("The Query of Queries table '" + table + "' is not a query object.").c_str());
        }
        createTableFromQuery(db, table, qv->m_query, schemas);
    }
    throwDatabaseError("Query of Queries: too many unresolvable tables.");
}

// CF-observable recordcount for a first non-result statement (only reachable
// for non-SELECT statements, which runQueryOfQueries rejects up front, but kept
// for parity with the file-backed execute() path).
void applyNonResultSemantics(sqlite3 *db, sqlite3_stmt *stmt,
                             long long &rowCount, long long &generatedKey,
                             bool &hasGeneratedKey)
{
    rowCount = sqlite3_changes(db);
    std::string up = sqlite3_sql(stmt) ? sqlite3_sql(stmt) : "";
    for (char &c : up) c = static_cast<char>(toupper(static_cast<unsigned char>(c)));
    bool isDml = up.find("INSERT") != std::string::npos ||
                 up.find("UPDATE") != std::string::npos ||
                 up.find("DELETE") != std::string::npos ||
                 up.find("REPLACE") != std::string::npos;
    if (rowCount == 0 && !isDml) rowCount = 1;
    if (up.find("INSERT") != std::string::npos) {
        sqlite3_int64 lid = sqlite3_last_insert_rowid(db);
        if (lid != 0) {
            generatedKey = static_cast<long long>(lid);
            hasGeneratedKey = true;
        }
    }
}

cfvariant sqliteCellToVariant(sqlite3_stmt *stmt, int col)
{
    switch (sqlite3_column_type(stmt, col)) {
    case SQLITE_INTEGER: {
        sqlite3_int64 v = sqlite3_column_int64(stmt, col);
        if (v >= INT32_MIN && v <= INT32_MAX) return cfvariant(static_cast<int>(v));
        cfvariant c(cfvariant::Long);
        c.m_long = static_cast<long long>(v);
        return c;
    }
    case SQLITE_FLOAT: {
        cfvariant c(cfvariant::Float);
        c.m_double = sqlite3_column_double(stmt, col);
        return c;
    }
    case SQLITE_TEXT: {
        const unsigned char *t = sqlite3_column_text(stmt, col);
        return t ? cfvariant(reinterpret_cast<const char*>(t)) : cfvariant(cfvariant::Null);
    }
    case SQLITE_BLOB: {
        cfvariant c(cfvariant::Binary);
        const void *b = sqlite3_column_blob(stmt, col);
        int n = sqlite3_column_bytes(stmt, col);
        const auto *bytes = static_cast<const std::byte*>(b);
        c.m_binary->assign(bytes, bytes + n);
        return c;
    }
    default:
        return cfvariant(cfvariant::Null);
    }
}

// A CFML type hint for a result column, derived from its cells.
std::string deriveColumnType(const std::vector<cfvariant> &values)
{
    bool anyFloat = false, anyInt = false, anyBool = false, anyDate = false, anyBin = false;
    for (const auto &v : values) {
        switch (v.m_type) {
        case cfvariant::Float: anyFloat = true; break;
        case cfvariant::Number:
        case cfvariant::Long: anyInt = true; break;
        case cfvariant::Boolean: anyBool = true; break;
        case cfvariant::DateTime: anyDate = true; break;
        case cfvariant::Binary: anyBin = true; break;
        default: break;
        }
    }
    if (anyFloat) return "DOUBLE";
    if (anyDate) return "DATE";
    if (anyBool) return "BOOLEAN";
    if (anyInt) return "INTEGER";
    if (anyBin) return "BLOB";
    return "VARCHAR";
}

} // namespace

DBResult runQueryOfQueries(const std::string &sqlIn, long long maxrows,
                           const QoQResolver &resolver)
{
    sqlite3 *db = nullptr;
    if (sqlite3_open(":memory:", &db) != SQLITE_OK) {
        std::string msg = db ? sqlite3_errmsg(db) : "sqlite3_open failed";
        if (db) sqlite3_close(db);
        throwDatabaseError(msg);
    }
    struct DbCloser { sqlite3 *db; ~DbCloser() { if (db) sqlite3_close(db); } } closer{db};

    // Reject non-SELECT statements up front: DML would mutate throwaway copies
    // of the source queries (CF writes back into the originals), so it must not
    // run silently wrong.
    {
        std::string head = ciKey(trimStr(sqlIn));
        if (!head.empty() && head.rfind("select", 0) != 0 &&
            head.rfind("with", 0) != 0 && head[0] != '(') {
            throw webstrada::exception("Database", "Error Executing Database Query.",
                "Query of Queries currently supports SELECT statements only "
                "(INSERT/UPDATE/DELETE are not implemented).");
        }
    }

    // Materialize every table the statements reference. The rewrite below needs
    // the column affinities, which are only known once a table exists, so this
    // pass walks the statements with the original SQL (a `+` on text columns
    // still prepares in SQLite) and the executed statements are prepared again
    // from the rewritten text afterwards.
    std::set<std::string> created;  // case-insensitive materialized names
    SchemaMap schemas;
    {
        const char *pz = sqlIn.c_str();
        while (true) {
            sqlite3_stmt *stmt = prepareAndMaterialize(db, pz, created, schemas, resolver);
            if (!stmt) break;
            sqlite3_finalize(stmt);
        }
    }

    std::string sql = rewriteConcatOperators(sqlIn, schemas);

    DBResult result;
    const char *pzTail = sql.c_str();
    bool first = true;
    while (true) {
        sqlite3_stmt *stmt = prepareAndMaterialize(db, pzTail, created, schemas, resolver);
        if (!stmt) break;  // only whitespace / comments remain

        int colCount = sqlite3_column_count(stmt);
        int rc = SQLITE_OK;
        if (first && colCount > 0) {
            for (int c = 0; c < colCount; c++) {
                DBColumn col;
                const char *name = sqlite3_column_name(stmt, c);
                col.name = name ? name : "";
                col.type = "varchar";
                result.columns.push_back(std::move(col));
            }
            rc = sqlite3_step(stmt);
            while (rc == SQLITE_ROW) {
                if (maxrows < 0 || result.rowCount < maxrows) {
                    for (int c = 0; c < colCount; c++) {
                        result.columns[c].values.push_back(sqliteCellToVariant(stmt, c));
                    }
                    result.rowCount++;
                }
                rc = sqlite3_step(stmt);
            }
            // Refine the per-column type hints from the actual cells.
            for (size_t c = 0; c < result.columns.size(); c++) {
                result.columns[c].type = deriveColumnType(result.columns[c].values);
            }
        } else {
            rc = sqlite3_step(stmt);
            while (rc == SQLITE_ROW) rc = sqlite3_step(stmt);
            if (first) {
                applyNonResultSemantics(db, stmt, result.rowCount,
                                        result.generatedKey, result.hasGeneratedKey);
            }
        }
        sqlite3_finalize(stmt);
        first = false;
        if (rc != SQLITE_DONE && rc != SQLITE_ROW) {
            throwDatabaseError(sqlite3_errmsg(db));
        }
    }
    return result;
}

} // namespace db
} // namespace webstrada
