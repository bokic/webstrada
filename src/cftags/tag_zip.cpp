/**
 * @file tag_zip.cpp
 * @brief <cfzip> / <cfzipparam> runtime (cf_zip_begin / cf_zip_param /
 * cf_zip_end).
 *
 * Implements the zip/unzip/list/read/readBinary/delete actions of <cfzip> with
 * ColdFusion semantics (the docs' attributes; the entry naming follows
 * java.util.zip / Apache Commons Compress conventions used by CF's ZipTag).
 * The <cfzipparam> children accumulate into a per-thread context
 * (cf_zip_begin pushes it, cf_zip_param appends, cf_zip_end pops it and runs
 * the action) like <cfstoredproc>/<cfprocparam>.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>
#include <webstrada/exceptions.h>
#include <webstrada/string.h>

#include <minizip/unzip.h>
#include <minizip/zip.h>

#include <algorithm>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

namespace cfml {

namespace {

struct ZipParam {
    std::string source;      // file/dir to add (zip action) or entry path
    std::string content;     // string content written to the entry (zip)
    std::string entrypath;   // entry path (zip content / unzip / delete)
    std::string filter;
    std::string prefix;
    std::string charset;
    bool recurse = true;
};

struct ZipCtx {
    std::vector<ZipParam> params;
};

thread_local std::vector<ZipCtx*> g_zipCtxs;

// Read an attribute from the evaluated attribute struct (case-insensitive).
const cfvariant *attrOf(const cfvariant *attrs, const char *key)
{
    if (!attrs || attrs->m_type != cfvariant::Struct || !attrs->m_struct) return nullptr;
    string k(key);
    auto it = attrs->m_struct->find(k);
    return it == attrs->m_struct->end() ? nullptr : &it->second;
}

std::string attrStr(const cfvariant *attrs, const char *key)
{
    const cfvariant *v = attrOf(attrs, key);
    return v ? safe_to_std_string(*v) : std::string();
}

bool attrBool(const cfvariant *attrs, const char *key, bool def)
{
    const cfvariant *v = attrOf(attrs, key);
    return v ? cfmlBoolean(v, def) : def;
}

[[noreturn]] void throwApp(const std::string &message, const std::string &detail = "")
{
    throw webstrada::exception(webstrada::string("Application"),
        webstrada::string(message.c_str()), webstrada::string(detail.c_str()));
}

// Normalize entry paths to forward slashes (Java zip entries use '/').
std::string normalizeEntry(const std::string &p)
{
    std::string r = p;
    std::replace(r.begin(), r.end(), '\\', '/');
    while (r.size() >= 2 && r[0] == '/' && r[1] == '/') r.erase(0, 1);
    return r;
}

// Convert a DOS date to CF's serial days (midnight local).
double dosDateToDays(uLong dosDate)
{
    int day = (dosDate & 0x1F);
    int month = ((dosDate >> 5) & 0x0F);
    int year = ((dosDate >> 9) & 0x7F) + 1980;
    int second = (dosDate & 0x1F) * 2;
    int minute = ((dosDate >> 5) & 0x3F);
    int hour = ((dosDate >> 11) & 0x1F);
    struct tm tm;
    memset(&tm, 0, sizeof(tm));
    tm.tm_year = year - 1900;
    tm.tm_mon = month - 1;
    tm.tm_mday = day;
    tm.tm_hour = hour;
    tm.tm_min = minute;
    tm.tm_sec = second;
    time_t t = timegm(&tm);
    return (double)t / 86400.0;
}

// CF's extension filter applied to entry/file names (the cfzip filter is the
// same pipe-delimited wildcard syntax as cfdirectory).
bool nameMatchesFilter(const std::string &name, const std::string &filter)
{
    if (filter.empty()) return true;
    // split on '|'
    size_t pos = 0;
    while (pos <= filter.size()) {
        size_t bar = filter.find('|', pos);
        std::string pat = bar == std::string::npos ? filter.substr(pos) : filter.substr(pos, bar - pos);
        std::string regex;
        int prevAsterisk = -2;
        for (size_t i = 0; i < pat.size(); i++) {
            char c = pat[i];
            if (c == '*') {
                if ((int)i != prevAsterisk + 1) regex += ".*";
                prevAsterisk = (int)i;
            } else if (c == '?') {
                regex += ".";
            } else if (std::string(".+{}()[]\\^$").find(c) != std::string::npos) {
                regex += "\\";
                regex += c;
            } else {
                regex += c;
            }
        }
        if (!regex.empty()) {
            std::regex re(regex);
            std::string base = name;
            size_t slash = base.find_last_of('/');
            if (slash != std::string::npos) base = base.substr(slash + 1);
            if (std::regex_match(base, re)) return true;
        }
        if (bar == std::string::npos) break;
        pos = bar + 1;
    }
    return false;
}

// ---- archive write helpers -------------------------------------------------

void zipAddBytes(zipFile zf, const std::string &entryName, const char *data, size_t len,
                 const std::string &comment)
{
    zip_fileinfo zi;
    memset(&zi, 0, sizeof(zi));
    zi.dosDate = 0;   // current time
    int err = zipOpenNewFileInZip(zf, entryName.c_str(), &zi,
                                  NULL, 0, NULL, 0,
                                  comment.empty() ? NULL : comment.c_str(), Z_DEFLATED, Z_DEFAULT_COMPRESSION);
    if (err != ZIP_OK) throwApp("The ZIP file entry " + entryName + " could not be created.");
    if (len > 0) {
        unsigned written = zipWriteInFileInZip(zf, data, static_cast<unsigned>(len));
        if (written != 0) {
            zipCloseFileInZip(zf);
            throwApp("The ZIP file entry " + entryName + " could not be written.");
        }
    }
    zipCloseFileInZip(zf);
}

// Recursively add a directory's files to the archive. `baseRel` is the entry
// path prefix of `dir` relative to the source root ("" for the root); it
// already carries the `prefix` (a top-level directory). With storePath=no the
// pathnames are dropped and each file lands at the root by its filename.
void zipAddDir(zipFile zf, const std::filesystem::path &dir, const std::string &baseRel,
               const std::string &filter, bool recurse, bool storePath,
               bool *addedAny)
{
    // Add the directory entry itself (trailing '/', like java.util.zip) when its
    // name is stored.
    if (storePath && !baseRel.empty()) {
        std::string dirEntry = baseRel;
        if (dirEntry.back() != '/') dirEntry += "/";
        zipAddBytes(zf, normalizeEntry(dirEntry), nullptr, 0, "");
    }
    for (auto it = std::filesystem::directory_iterator(dir);
         it != std::filesystem::directory_iterator(); ++it) {
        const auto &entry = *it;
        std::filesystem::path p = entry.path();
        std::string name = p.filename().string();
        std::string rel = baseRel.empty() ? name : baseRel + "/" + name;
        if (entry.is_directory()) {
            if (recurse) {
                zipAddDir(zf, p, rel, filter, recurse, storePath, addedAny);
            }
        } else if (entry.is_regular_file()) {
            if (!filter.empty() && !nameMatchesFilter(name, filter)) continue;
            std::ifstream in(p, std::ios::binary);
            std::vector<char> buf((std::istreambuf_iterator<char>(in)),
                                  std::istreambuf_iterator<char>());
            std::string entryName = storePath ? rel : name;
            zipAddBytes(zf, normalizeEntry(entryName), buf.data(), buf.size(), "");
            if (addedAny) *addedAny = true;
        }
    }
}

} // namespace

// Push a fresh <cfzip> context.
void cf_zip_begin()
{
    g_zipCtxs.push_back(new ZipCtx());
}

// <cfzipparam> runtime: append a parameter to the current context.
void cf_zip_param(const cfvariant *source, const cfvariant *content,
                  const cfvariant *entrypath, const cfvariant *filter,
                  const cfvariant *prefix, const cfvariant *recurse,
                  const cfvariant *charset, const cfvariant *encryptionalgorithm,
                  const cfvariant *password)
{
    if (g_zipCtxs.empty()) {
        throw webstrada::exception(webstrada::string("Application"),
            webstrada::string("cfzipparam is only valid inside a cfzip tag."),
            webstrada::string(""));
    }
    ZipParam p;
    if (source) p.source = safe_to_std_string(*source);
    if (content) p.content = safe_to_std_string(*content);
    if (entrypath) p.entrypath = safe_to_std_string(*entrypath);
    if (filter) p.filter = safe_to_std_string(*filter);
    if (prefix) p.prefix = safe_to_std_string(*prefix);
    if (recurse) p.recurse = cfmlBoolean(recurse, true);
    if (charset) p.charset = safe_to_std_string(*charset);
    // encryptionalgorithm/password are accepted but not implemented (plain zip).
    (void)encryptionalgorithm;
    (void)password;
    g_zipCtxs.back()->params.push_back(std::move(p));
}

// Per-request reset so an exception unwinding past cf_zip_end cannot leak a
// context (called from scope_begin / the request teardown).
void zip_ctx_clear()
{
    for (auto *c : g_zipCtxs) delete c;
    g_zipCtxs.clear();
}

// <cfzip> runtime entry. `attrs` holds the evaluated attributes (lowercased
// keys); the scopes are passed so the list/read/readBinary actions can assign
// their result variables.
void cf_zip_end(const cfvariant *attrs, void *cgi, void *server, void *cookie,
                void *application, void *session, void *url, void *form,
                void *variables)
{
    if (g_zipCtxs.empty()) {
        throw webstrada::exception(webstrada::string("Application"),
            webstrada::string("cfzip: no active zip context."), webstrada::string(""));
    }
    ZipCtx *ctx = g_zipCtxs.back();
    g_zipCtxs.pop_back();
    std::unique_ptr<ZipCtx> guard(ctx);

    std::string action = attrStr(attrs, "action");
    if (action.empty()) action = "zip";
    std::string lowerAction;
    for (char c : action) lowerAction += static_cast<char>(tolower((unsigned char)c));

    std::string file = attrStr(attrs, "file");
    std::string source = attrStr(attrs, "source");
    std::string destination = attrStr(attrs, "destination");
    std::string entrypath = attrStr(attrs, "entrypath");
    std::string name = attrStr(attrs, "name");
    std::string variable = attrStr(attrs, "variable");
    std::string charset = attrStr(attrs, "charset");
    std::string filter = attrStr(attrs, "filter");
    std::string prefix = attrStr(attrs, "prefix");
    std::string password = attrStr(attrs, "password");
    bool recurse = attrBool(attrs, "recurse", true);
    bool overwrite = attrBool(attrs, "overwrite", false);
    bool showDirectory = attrBool(attrs, "showDirectory", false);
    bool storePath = attrBool(attrs, "storePath", true);

    // A relative `file` is created in the OS temp directory like CF.
    std::filesystem::path filePath(file);
    if (!filePath.is_absolute()) {
        filePath = std::filesystem::temp_directory_path() / file;
    }
    std::string zipPath = filePath.string();

    if (lowerAction == "list") {
        unzFile zf = unzOpen(zipPath.c_str());
        if (!zf) {
            throwApp("Exception encountered while reading the file " + zipPath + ".",
                     "Ensure that the file is a valid zip file and it is accessible.  Cause : "
                     "net.lingala.zip4j.exception.ZipException: zip file does not exist");
        }
        cfvariant q(cfvariant::Query);
        QueryData *qd = q.m_query;
        static const char *colNames[] = {"NAME", "DIRECTORY", "SIZE", "COMPRESSEDSIZE",
                                         "TYPE", "DATELASTMODIFIED", "COMMENT", "CRC",
                                         "ENCRYPTIONALGORITHM"};
        static const char *colTypes[] = {"VARCHAR", "VARCHAR", "BIGINT", "BIGINT",
                                         "VARCHAR", "DATE", "VARCHAR", "BIGINT", "VARCHAR"};
        for (int i = 0; i < 9; i++) {
            QueryColumn c;
            c.name = colNames[i];
            c.type = colTypes[i];
            qd->columns.push_back(std::move(c));
        }
        std::vector<cfvariant> row[9];
        int err = UNZ_OK;
        for (err = unzGoToFirstFile(zf); err == UNZ_OK; err = unzGoToNextFile(zf)) {
            unz_file_info64 fi;
            char nameBuf[4096];
            char commentBuf[4096];
            memset(&fi, 0, sizeof(fi));
            int r = unzGetCurrentFileInfo64(zf, &fi, nameBuf, sizeof(nameBuf), NULL, 0,
                                            commentBuf, sizeof(commentBuf));
            if (r != UNZ_OK) continue;
            std::string entryName = nameBuf;
            bool isDir = !entryName.empty() && entryName.back() == '/';
            // CF 2025's cfzip list: the NAME column is the full entry path
            // (directory entries keep the trailing '/'), and the DIRECTORY
            // column is the entry's parent path for files ("" for directory
            // entries and root-level files).
            std::string base = entryName;
            std::string dir;
            if (isDir) {
                dir.clear();
            } else {
                size_t slash = base.find_last_of('/');
                if (slash != std::string::npos) {
                    dir = base.substr(0, slash);
                }
            }
            if (isDir && !showDirectory) continue;
            if (!filter.empty() && !nameMatchesFilter(entryName, filter)) continue;

            row[0].emplace_back(cfvariant(base.c_str()));
            row[1].emplace_back(cfvariant(dir.c_str()));
            {
                cfvariant v(cfvariant::Long);
                v.m_long = (long long)fi.uncompressed_size;
                row[2].push_back(v);
            }
            {
                cfvariant v(cfvariant::Long);
                v.m_long = (long long)fi.compressed_size;
                row[3].push_back(v);
            }
            row[4].emplace_back(cfvariant(isDir ? "dir" : "file"));
            if (fi.dosDate != 0) {
                cfvariant dt(cfvariant::DateTime);
                dt.m_double = dosDateToDays(fi.dosDate);
                row[5].push_back(dt);
            } else {
                row[5].emplace_back(cfvariant(""));
            }
            row[6].emplace_back(cfvariant(std::string(commentBuf).c_str()));
            {
                cfvariant v(cfvariant::Long);
                v.m_long = (long long)fi.crc;
                row[7].push_back(v);
            }
            row[8].emplace_back(cfvariant(""));
        }
        unzClose(zf);
        for (int i = 0; i < 9; i++) qd->columns[i].values = row[i];
        qd->m_rowCount = (int)row[0].size();
        cfvariant_assign(static_cast<const cfvariant*>(cgi), static_cast<const cfvariant*>(server),
                         static_cast<const cfvariant*>(cookie), static_cast<const cfvariant*>(application),
                         static_cast<const cfvariant*>(session), static_cast<const cfvariant*>(url),
                         static_cast<const cfvariant*>(form), static_cast<cfvariant*>(variables),
                         name.c_str(), &q);
        return;
    }

    if (lowerAction == "read" || lowerAction == "readbinary") {
        if (entrypath.empty()) {
            throwApp("Attribute validation error for CFZIP.",
                     "When the value of the ACTION attribute is " +
                     std::string(lowerAction == "read" ? "READ" : "READBINARY") +
                     ", it requires the attribute(s): ENTRYPATH.");
        }
        unzFile zf = unzOpen(zipPath.c_str());
        if (!zf) {
            throwApp("Exception encountered while reading the file " + zipPath + ".",
                     "Ensure that the file is a valid zip file and it is accessible.  Cause : "
                     "net.lingala.zip4j.exception.ZipException: zip file does not exist");
        }
        std::string target = normalizeEntry(entrypath);
        int err = unzLocateFile(zf, target.c_str(), 2 /* case sensitive */);
        if (err != UNZ_OK) {
            unzClose(zf);
            throwApp("The zip entry for path " + entrypath + " was not found in the zip file specified.");
        }
        unz_file_info64 fi;
        unzGetCurrentFileInfo64(zf, &fi, NULL, 0, NULL, 0, NULL, 0);
        if (unzOpenCurrentFile(zf) != UNZ_OK) {
            unzClose(zf);
            throwApp("The entry " + entrypath + " could not be read.");
        }
        std::vector<char> data((size_t)fi.uncompressed_size);
        size_t total = 0;
        if (!data.empty()) {
            int rd = unzReadCurrentFile(zf, data.data(), (unsigned)data.size());
            if (rd > 0) total = (size_t)rd;
        }
        unzCloseCurrentFile(zf);
        unzClose(zf);

        if (lowerAction == "read") {
            std::string text(data.data(), total);
            cfvariant val(text.c_str());
            cfvariant_assign(static_cast<const cfvariant*>(cgi), static_cast<const cfvariant*>(server),
                             static_cast<const cfvariant*>(cookie), static_cast<const cfvariant*>(application),
                             static_cast<const cfvariant*>(session), static_cast<const cfvariant*>(url),
                             static_cast<const cfvariant*>(form), static_cast<cfvariant*>(variables),
                             variable.c_str(), &val);
        } else {
            std::vector<std::byte> bytes;
            bytes.reserve(total);
            for (char c : data) bytes.push_back(static_cast<std::byte>(c));
            cfvariant val(cfvariant::Binary);
            *val.m_binary = std::move(bytes);
            cfvariant_assign(static_cast<const cfvariant*>(cgi), static_cast<const cfvariant*>(server),
                             static_cast<const cfvariant*>(cookie), static_cast<const cfvariant*>(application),
                             static_cast<const cfvariant*>(session), static_cast<const cfvariant*>(url),
                             static_cast<const cfvariant*>(form), static_cast<cfvariant*>(variables),
                             variable.c_str(), &val);
        }
        return;
    }

    if (lowerAction == "unzip") {
        if (destination.empty()) {
            throwApp("Attribute validation error for CFZIP.",
                     "When the value of the ACTION attribute is UNZIP, it requires "
                     "the attribute(s): DESTINATION.");
        }
        // The destination must already exist and be a directory (CF throws
        // otherwise).
        std::error_code dec;
        if (!std::filesystem::is_directory(destination, dec)) {
            throwApp("The destination " + destination + " specified in the cfzip tag is invalid.",
                     "The destination must be a directory and should be accessible.");
        }
        unzFile zf = unzOpen(zipPath.c_str());
        if (!zf) {
            throwApp("Exception encountered while reading the file " + zipPath + ".",
                     "Ensure that the file is a valid zip file and it is accessible.  Cause : "
                     "net.lingala.zip4j.exception.ZipException: zip file does not exist");
        }
        std::string singleEntry = normalizeEntry(entrypath);
        bool onlyEntry = !singleEntry.empty();
        int err = onlyEntry ? unzLocateFile(zf, singleEntry.c_str(), 2) : UNZ_OK;
        if (onlyEntry && err != UNZ_OK) {
            unzClose(zf);
            throwApp("The zip entry for path " + entrypath + " was not found in the zip file specified.");
        }
        int iter;
        for (iter = onlyEntry ? 0 : unzGoToFirstFile(zf); iter == UNZ_OK; ) {
            unz_file_info64 fi;
            char nameBuf[4096];
            memset(&fi, 0, sizeof(fi));
            unzGetCurrentFileInfo64(zf, &fi, nameBuf, sizeof(nameBuf), NULL, 0, NULL, 0);
            std::string entryName = nameBuf;
            std::string rel = normalizeEntry(entryName);
            if (rel.empty()) { if (!onlyEntry) { iter = unzGoToNextFile(zf); } else break; continue; }
            // path traversal guard
            std::string clean;
            {
                size_t pos = 0;
                while (pos < rel.size()) {
                    size_t slash = rel.find('/', pos);
                    std::string comp = slash == std::string::npos ? rel.substr(pos) : rel.substr(pos, slash - pos);
                    if (comp == "..") { clean.clear(); break; }
                    if (comp != "." && !comp.empty()) { if (!clean.empty()) clean += "/"; clean += comp; }
                    if (slash == std::string::npos) break;
                    pos = slash + 1;
                }
            }
            if (clean.empty()) { if (!onlyEntry) { iter = unzGoToNextFile(zf); } else break; continue; }
            if (!filter.empty() && !nameMatchesFilter(rel, filter)) {
                if (!onlyEntry) { iter = unzGoToNextFile(zf); } else break; continue;
            }
            bool isDir = rel.back() == '/';
            std::string outRel = storePath ? clean : (std::filesystem::path(clean).filename().string());
            if (outRel.empty()) { if (!onlyEntry) { iter = unzGoToNextFile(zf); } else break; continue; }
            std::filesystem::path target = std::filesystem::path(destination) / outRel;
            if (isDir) {
                std::filesystem::create_directories(target);
            } else {
                if (unzOpenCurrentFile(zf) == UNZ_OK) {
                    if (!std::filesystem::exists(target) || overwrite) {
                        std::filesystem::create_directories(target.parent_path());
                        std::ofstream out(target, std::ios::binary | std::ios::trunc);
                        char buf[8192];
                        int rd;
                        while ((rd = unzReadCurrentFile(zf, buf, sizeof(buf))) > 0) {
                            out.write(buf, rd);
                        }
                        out.close();
                    }
                    unzCloseCurrentFile(zf);
                }
            }
            if (onlyEntry) break;
            iter = unzGoToNextFile(zf);
        }
        unzClose(zf);
        return;
    }

    if (lowerAction == "delete") {
        unzFile zf = unzOpen(zipPath.c_str());
        if (!zf) {
            throwApp("Exception encountered while reading the file " + zipPath + ".",
                     "Ensure that the file is a valid zip file and it is accessible.  Cause : "
                     "net.lingala.zip4j.exception.ZipException: zip file does not exist");
        }
        // Collect the entries that survive.
        std::vector<std::pair<std::string, std::vector<char>>> keep;
        std::string target = normalizeEntry(entrypath);
        for (int iter = unzGoToFirstFile(zf); iter == UNZ_OK; iter = unzGoToNextFile(zf)) {
            unz_file_info64 fi;
            char nameBuf[4096];
            memset(&fi, 0, sizeof(fi));
            unzGetCurrentFileInfo64(zf, &fi, nameBuf, sizeof(nameBuf), NULL, 0, NULL, 0);
            std::string entryName = nameBuf;
            bool isDir = !entryName.empty() && entryName.back() == '/';
            std::string norm = normalizeEntry(entryName);
            if (!isDir && norm == target) continue;
            if (!isDir && target == (norm + "/")) continue;
            std::vector<char> data;
            if (!isDir && unzOpenCurrentFile(zf) == UNZ_OK) {
                data.resize((size_t)fi.uncompressed_size);
                size_t total = 0;
                if (!data.empty()) {
                    int rd = unzReadCurrentFile(zf, data.data(), (unsigned)data.size());
                    if (rd > 0) total = (size_t)rd;
                }
                data.resize(total);
                unzCloseCurrentFile(zf);
            }
            keep.push_back({entryName, std::move(data)});
        }
        unzClose(zf);

        zipFile zout = zipOpen(zipPath.c_str(), APPEND_STATUS_CREATE);
        if (!zout) throwApp("The zip file " + zipPath + " could not be created.");
        for (auto &e : keep) {
            zipAddBytes(zout, normalizeEntry(e.first), e.second.data(), e.second.size(), "");
        }
        zipClose(zout, NULL);
        return;
    }

    // action == "zip"
    {
        if (zipPath.empty()) {
            throwApp("Attribute validation error for CFZIP.",
                     "When the value of the ACTION attribute is ZIP, it requires "
                     "the attribute(s): FILE.");
        }
        int appendMode = APPEND_STATUS_CREATE;
        if (!overwrite && std::filesystem::exists(zipPath)) {
            appendMode = APPEND_STATUS_ADDINZIP;
        }
        zipFile zf = zipOpen(zipPath.c_str(), appendMode);
        if (!zf) throwApp("The zip file " + zipPath + " could not be created.");

        bool addedAny = false;
        // The cfzip source attribute: add the whole directory/file.
        if (!source.empty()) {
            std::filesystem::path src(source);
            std::error_code ec;
            if (std::filesystem::is_directory(src, ec)) {
                // CF stores the entries relative to the source directory (the
                // directory name itself is NOT part of the entry paths); a
                // prefix prepends a top-level directory.
                std::string baseRel = prefix;
                zipAddDir(zf, src, baseRel, filter, recurse, storePath, &addedAny);
            } else if (std::filesystem::is_regular_file(src, ec)) {
                if (filter.empty() || nameMatchesFilter(src.filename().string(), filter)) {
                    std::ifstream in(src, std::ios::binary);
                    std::vector<char> buf((std::istreambuf_iterator<char>(in)),
                                          std::istreambuf_iterator<char>());
                    std::string entryName;
                    if (!entrypath.empty()) {
                        entryName = entrypath;
                    } else if (storePath) {
                        entryName = src.filename().string();
                    } else {
                        entryName = src.filename().string();
                    }
                    if (!prefix.empty()) entryName = prefix + "/" + entryName;
                    zipAddBytes(zf, normalizeEntry(entryName), buf.data(), buf.size(), "");
                    addedAny = true;
                }
            }
        }

        // Each <cfzipparam>.
        for (const auto &p : ctx->params) {
            std::string pPrefix = p.prefix.empty() ? prefix : p.prefix;
            if (!p.content.empty()) {
                std::string entryName = p.entrypath;
                if (entryName.empty()) {
                    throwApp("Attribute validation error for CFZIP.",
                             "When the content attribute is specified for cfzipparam, "
                             "the entrypath attribute is required.");
                }
                if (!pPrefix.empty()) entryName = pPrefix + "/" + entryName;
                zipAddBytes(zf, normalizeEntry(entryName), p.content.data(), p.content.size(), "");
                addedAny = true;
                continue;
            }
            if (p.source.empty()) continue;
            std::filesystem::path src(p.source);
            std::error_code ec;
            if (std::filesystem::is_directory(src, ec)) {
                std::string baseRel = p.entrypath;
                if (baseRel.empty()) baseRel = pPrefix;
                else if (!pPrefix.empty()) baseRel = pPrefix + "/" + baseRel;
                zipAddDir(zf, src, baseRel, p.filter.empty() ? filter : p.filter,
                          p.recurse, storePath, &addedAny);
            } else if (std::filesystem::is_regular_file(src, ec)) {
                std::string filt = p.filter.empty() ? filter : p.filter;
                if (filt.empty() || nameMatchesFilter(src.filename().string(), filt)) {
                    std::ifstream in(src, std::ios::binary);
                    std::vector<char> buf((std::istreambuf_iterator<char>(in)),
                                          std::istreambuf_iterator<char>());
                    std::string entryName;
                    if (!p.entrypath.empty()) entryName = p.entrypath;
                    else entryName = src.filename().string();
                    if (!pPrefix.empty()) entryName = pPrefix + "/" + entryName;
                    zipAddBytes(zf, normalizeEntry(entryName), buf.data(), buf.size(), "");
                    addedAny = true;
                }
            }
        }
        (void)addedAny;
        zipClose(zf, NULL);
        return;
    }
}

} // namespace cfml
