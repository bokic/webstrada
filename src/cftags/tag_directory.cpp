/**
 * @file tag_directory.cpp
 * @brief <cfdirectory> runtime (cf_directory_tag).
 *
 * Implements the list/create/delete/rename/copy actions of <cfdirectory> with
 * Adobe ColdFusion semantics (ported from the decompiled DirectoryTag /
 * FileUtils / FileListTable / MultiColumnSorter). The list action builds the
 * FileListTable query (NAME, SIZE, TYPE, DATELASTMODIFIED, ATTRIBUTES, MODE,
 * DIRECTORY, LINK columns) and assigns it to the `name` variable; the other
 * actions mutate the file system. Attribute validation (unknown attributes,
 * invalid ACTION value for static literals) is performed at compile time in
 * the codegen; the runtime validates the action/type at run time for dynamic
 * values and throws CF's catchable Application errors.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>
#include <webstrada/exceptions.h>
#include <webstrada/string.h>

#include <algorithm>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <regex>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

namespace cfml {

namespace {

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

// Throws CF's catchable Application error.
[[noreturn]] void throwApp(const std::string &message, const std::string &detail = "")
{
    throw webstrada::exception(webstrada::string("Application"),
        webstrada::string(message.c_str()), webstrada::string(detail.c_str()));
}

// Convert the `type` attribute ("all"/"dir"/"file") to CF's integer. An
// invalid value throws CF's InvalidTypeAttributeException.
int typeToInt(const std::string &t)
{
    std::string v = t;
    v = v.substr(v.find_first_not_of(" \t\r\n"));
    size_t e = v.find_last_not_of(" \t\r\n");
    if (e != std::string::npos) v = v.substr(0, e + 1);
    std::string up;
    for (char c : v) up += static_cast<char>(tolower((unsigned char)c));
    if (up == "dir") return 1;
    if (up == "file") return 2;
    if (up == "all") return 0;
    throwApp("Invalid value for the type attribute: " + v + ".",
             "Valid values are file, dir, and all.");
}

// CF's ExtensionFilter: converts a pipe-delimited wildcard pattern to a regex
// and matches the file NAME. '*' -> '.*' (consecutive stars collapse), '?' ->
// '.', regex metacharacters are escaped.
class ExtensionFilter {
public:
    ExtensionFilter(const std::string &cfFilter) {
        std::string regex;
        int prevAsterisk = -2;
        for (size_t i = 0; i < cfFilter.size(); i++) {
            char c = cfFilter[i];
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
        if (regex.empty()) regex = ".*";
        m_pattern = std::regex(regex);
        m_valid = true;
    }
    bool accept(const std::string &name) const {
        return std::regex_match(name, m_pattern);
    }
    bool valid() const { return m_valid; }
private:
    std::regex m_pattern;
    bool m_valid = false;
};

// File metadata snapshot used by the sort comparator.
struct FileMeta {
    std::string name;       // display name (filename, or relative path for listinfo=name+recurse)
    std::string sortName;   // filename (last component) used for NAME sorting, like CF's File.getName()
    std::string dir;       // parent path ("" when none)
    std::string absPath;
    long long size = 0;
    bool isDir = false;
    bool isFile = false;
    long long lastModified = 0;
    std::string attributes;
    bool isLink = false;
};

// CF's MultiColumnSorter: comma-delimited sort tokens (uppercased), each
// "COLUMN" or "COLUMN ASC"/"COLUMN DESC". A column contributes only when all
// previous columns tied. NAME/DIRECTORY compare case-insensitively, SIZE and
// DATELASTMODIFIED numerically, TYPE puts directories first, ATTRIBUTES
// compares case-sensitively; unknown columns are ignored.
class MultiColumnSorter {
public:
    MultiColumnSorter(const std::string &sort) {
        std::string up;
        for (char c : sort) up += static_cast<char>(toupper((unsigned char)c));
        size_t pos = 0;
        auto trim = [](std::string s) {
            size_t b = s.find_first_not_of(" \t\r\n");
            if (b == std::string::npos) return std::string();
            size_t e = s.find_last_not_of(" \t\r\n");
            return s.substr(b, e - b + 1);
        };
        while (pos <= up.size()) {
            size_t comma = up.find(',', pos);
            std::string tok = comma == std::string::npos ? up.substr(pos) : up.substr(pos, comma - pos);
            tok = trim(tok);
            if (!tok.empty()) {
                bool desc = false;
                if (tok.size() >= 4 && tok.substr(tok.size() - 4) == "DESC") {
                    desc = true;
                    tok = trim(tok.substr(0, tok.size() - 4));
                } else if (tok.size() >= 3 && tok.substr(tok.size() - 3) == "ASC") {
                    tok = trim(tok.substr(0, tok.size() - 3));
                }
                if (!tok.empty()) m_cols.push_back({tok, desc});
            }
            if (comma == std::string::npos) break;
            pos = comma + 1;
        }
    }

    int compare(const FileMeta &a, const FileMeta &b) const {
        for (const auto &c : m_cols) {
            int r = 0;
            if (c.name == "NAME") {
                r = strcasecmp(a.sortName.c_str(), b.sortName.c_str());
            } else if (c.name == "DIRECTORY") {
                r = strcasecmp(a.dir.c_str(), b.dir.c_str());
            } else if (c.name == "SIZE") {
                r = a.size < b.size ? -1 : a.size > b.size ? 1 : 0;
            } else if (c.name == "DATELASTMODIFIED") {
                r = a.lastModified < b.lastModified ? -1 : a.lastModified > b.lastModified ? 1 : 0;
            } else if (c.name == "TYPE") {
                if (a.isDir != b.isDir) r = a.isDir ? -1 : 1;
            } else if (c.name == "ATTRIBUTES") {
                r = a.attributes.compare(b.attributes);
            }
            if (c.desc) r = -r;
            if (r != 0) return r;
        }
        return 0;
    }
    bool hasColumns() const { return !m_cols.empty(); }
    size_t columnCount() const { return m_cols.size(); }

private:
    struct Col { std::string name; bool desc; };
    std::vector<Col> m_cols;
};

std::string lastPathComponent(const std::string &p)
{
    size_t slash = p.find_last_of('/');
    return slash == std::string::npos ? p : p.substr(slash + 1);
}

bool isDir(const std::filesystem::directory_entry &e) { return e.is_directory(); }

// Recursively collect entries like CF's recursiveGetFiles: with a filter, the
// current directory's matching files are added first, then each subdirectory is
// recursed (and matched against the filter too when it is non-null).
void recursiveGetFiles(std::vector<std::filesystem::directory_entry> &list,
                       const std::filesystem::path &dir, const ExtensionFilter *filter)
{
    std::vector<std::filesystem::directory_entry> subdirs;
    for (auto it = std::filesystem::directory_iterator(dir, std::filesystem::directory_options::skip_permission_denied);
         it != std::filesystem::directory_iterator(); ++it) {
        const auto &entry = *it;
        if (filter) {
            if (filter->accept(entry.path().filename().string())) {
                list.push_back(entry);
            }
        } else {
            list.push_back(entry);
        }
        if (entry.is_directory()) subdirs.push_back(entry);
    }
    for (const auto &sd : subdirs) {
        recursiveGetFiles(list, sd.path(), filter);
    }
}

// The <cfdirectory> list query builder. `dirPath` is the directory being listed
// (used for the relative-path form of listinfo="name" with recurse).
cfvariant buildListQuery(const std::vector<FileMeta> &files, bool listNamesOnly,
                         int filetype, const std::string &dirPath)
{
    cfvariant q(cfvariant::Query);
    QueryData *qd = q.m_query;
    if (listNamesOnly) {
        QueryColumn c;
        c.name = "NAME";
        c.type = "VARCHAR";
        for (auto &f : files) {
            if (filetype == 0 || (filetype == 1 && f.isDir) || (filetype == 2 && f.isFile)) {
                c.values.emplace_back(cfvariant(f.name.c_str()));
            }
        }
        qd->columns.push_back(std::move(c));
    } else {
        static const char *colNames[] = {"NAME", "SIZE", "TYPE", "DATELASTMODIFIED",
                                         "ATTRIBUTES", "MODE", "DIRECTORY", "LINK"};
        static const char *colTypes[] = {"VARCHAR", "BIGINT", "VARCHAR", "DATE",
                                         "VARCHAR", "VARCHAR", "VARCHAR", "VARCHAR"};
        for (int i = 0; i < 8; i++) {
            QueryColumn c;
            c.name = colNames[i];
            c.type = colTypes[i];
            qd->columns.push_back(std::move(c));
        }
        for (auto &f : files) {
            if (filetype != 0 && ((filetype == 1) != f.isDir)) continue;
            qd->columns[0].values.emplace_back(cfvariant(f.name.c_str()));
            {
                cfvariant sz(cfvariant::Long);
                sz.m_long = f.size;
                qd->columns[1].values.push_back(sz);
            }
            qd->columns[2].values.emplace_back(cfvariant(f.isDir ? "Dir" : "File"));
            if (f.lastModified > 0) {
                cfvariant dt(cfvariant::DateTime);
                dt.m_double = f.lastModified / 86400.0;
                dt.m_odbcStyle = 3;   // CF QueryTable java.util.Date rendering
                qd->columns[3].values.push_back(dt);
            } else {
                qd->columns[3].values.emplace_back(cfvariant(""));
            }
            qd->columns[4].values.emplace_back(cfvariant(f.attributes.c_str()));
            qd->columns[5].values.emplace_back(cfvariant(""));
            qd->columns[6].values.emplace_back(cfvariant(f.dir.c_str()));
            {
                // Link: CF's FileListTable stores a java Boolean (renders NO/YES).
                cfvariant link(cfvariant::Boolean);
                link.m_bool = f.isLink;
                qd->columns[7].values.push_back(link);
            }
        }
    }
    qd->m_rowCount = qd->columns.empty() ? 0 : (int)qd->columns[0].values.size();
    return q;
}

} // namespace

// <cfdirectory> runtime entry. `attrs` holds the evaluated attributes with
// lowercased keys; the CFML scopes are passed so the list action can assign the
// `name` query variable like <cfquery>/<cfdbinfo>.
void cf_directory_tag(const cfvariant *attrs, void *cgi, void *server, void *cookie,
                      void *application, void *session, void *url, void *form,
                      void *variables)
{
    std::string action = attrStr(attrs, "action");
    if (action.empty()) action = "list";
    std::string directory = attrStr(attrs, "directory");
    std::string lowerAction;
    for (char c : action) lowerAction += static_cast<char>(tolower((unsigned char)c));

    if (lowerAction == "list") {
        std::string name = attrStr(attrs, "name");
        bool recurse = attrBool(attrs, "recurse", false);
        std::string filterStr = attrStr(attrs, "filter");
        std::string sortStr = attrStr(attrs, "sort");
        std::string typeStr = attrStr(attrs, "type");
        std::string listinfo = attrStr(attrs, "listinfo");
        int filetype = typeToInt(typeStr.empty() ? "all" : typeStr);
        bool listNamesOnly = false;
        for (char &c : listinfo) c = static_cast<char>(tolower((unsigned char)c));
        if (listinfo == "name") listNamesOnly = true;

        std::vector<FileMeta> metas;
        // Listing a non-existent directory returns an empty query (CF catches
        // InvalidDirectoryException). A non-readable one throws.
        try {
            std::filesystem::path dirPath(directory);
            std::error_code ec;
            bool isDirFs = std::filesystem::is_directory(dirPath, ec);
            if (ec || !isDirFs) {
                // invalid directory -> empty result
            } else if (access(directory.c_str(), R_OK) != 0) {
                throwApp("The specified directory " + directory + " cannot be read.",
                         "The most likely cause of this error is that " + directory +
                         " is not readable on your file system.");
            } else {
                std::unique_ptr<ExtensionFilter> filter;
                if (!filterStr.empty() && filterStr.find_first_not_of(" \t\r\n") != std::string::npos) {
                    filter.reset(new ExtensionFilter(filterStr));
                }
                std::vector<std::filesystem::directory_entry> entries;
                if (recurse) {
                    recursiveGetFiles(entries, dirPath, filter.get());
                } else {
                    if (filter) {
                        for (auto it = std::filesystem::directory_iterator(dirPath);
                             it != std::filesystem::directory_iterator(); ++it) {
                            if (filter->accept(it->path().filename().string())) {
                                entries.push_back(*it);
                            }
                        }
                    } else {
                        for (auto it = std::filesystem::directory_iterator(dirPath);
                             it != std::filesystem::directory_iterator(); ++it) {
                            entries.push_back(*it);
                        }
                    }
                }
                std::string dirStr = directory;
                if (!dirStr.empty() && dirStr.back() != '/') dirStr += "/";
                std::string relPrefix = dirStr;
                for (auto &entry : entries) {
                    FileMeta m;
                    std::error_code sec;
                    std::filesystem::path p = entry.path();
                    std::string full = p.string();
                    m.absPath = full;
                    m.sortName = p.filename().string();
                    if (listNamesOnly && recurse) {
                        std::string rel = full;
                        if (rel.compare(0, relPrefix.size(), relPrefix) == 0) {
                            rel = rel.substr(relPrefix.size());
                        } else {
                            // absolute path prefix mismatch: use the filename
                            rel = p.filename().string();
                        }
                        std::replace(rel.begin(), rel.end(), '\\', '/');
                        m.name = rel;
                    } else {
                        m.name = p.filename().string();
                    }
                    m.isDir = entry.is_directory(sec);
                    m.isFile = entry.is_regular_file(sec);
                    if (m.isFile) m.size = entry.file_size(sec);
                    struct stat stbuf;
                    if (::stat(full.c_str(), &stbuf) == 0) {
                        m.lastModified = (long long)stbuf.st_mtime;
                    } else {
                        m.lastModified = 0;
                    }
                    std::string parent = p.parent_path().string();
                    m.dir = parent;
                    m.isLink = std::filesystem::is_symlink(entry.symlink_status(sec));
                    metas.push_back(std::move(m));
                }
            }
        } catch (const std::filesystem::filesystem_error &) {
            // treat as empty listing
            metas.clear();
        }

        if (!sortStr.empty() && metas.size() > 1) {
            MultiColumnSorter sorter(sortStr);
            if (sorter.hasColumns()) {
                std::stable_sort(metas.begin(), metas.end(),
                    [&](const FileMeta &a, const FileMeta &b) { return sorter.compare(a, b) < 0; });
            }
        }

        cfvariant query = buildListQuery(metas, listNamesOnly, filetype, directory);
        cfvariant_assign(static_cast<const cfvariant*>(cgi), static_cast<const cfvariant*>(server),
                         static_cast<const cfvariant*>(cookie), static_cast<const cfvariant*>(application),
                         static_cast<const cfvariant*>(session), static_cast<const cfvariant*>(url),
                         static_cast<const cfvariant*>(form), static_cast<cfvariant*>(variables),
                         name.c_str(), &query);
        return;
    }

    if (lowerAction == "create") {
        std::error_code ec;
        bool existsDir = std::filesystem::is_directory(directory, ec);
        if (existsDir) {
            throwApp("The specified directory " + directory + " could not be created.",
                     "The most likely cause of this error is that " + directory +
                     " already exists on your file system.");
        }
        if (std::filesystem::exists(directory)) {
            throwApp("The specified directory " + directory + " could not be created.",
                     "The most likely cause of this error is that " + directory +
                     " is a file on your file system.");
        }
        if (!std::filesystem::create_directories(directory, ec) || ec) {
            throwApp("The specified directory " + directory + " could not be created.",
                     "The most likely cause of this error is that " + directory +
                     " already exists on your file system.");
        }
        std::string modeStr = attrStr(attrs, "mode");
        if (!modeStr.empty()) {
            std::string m = modeStr;
            m = m.substr(m.find_first_not_of(" \t\r\n"));
            if (!m.empty()) {
                long octal = 0;
                bool ok = !m.empty();
                for (char c : m) {
                    if (c < '0' || c > '7') { ok = false; break; }
                    octal = octal * 8 + (c - '0');
                }
                if (ok) ::chmod(directory.c_str(), static_cast<mode_t>(octal));
            }
        }
        return;
    }

    if (lowerAction == "delete") {
        bool recurse = attrBool(attrs, "recurse", false);
        std::error_code ec;
        if (!std::filesystem::is_directory(directory, ec)) {
            throwApp("The specified directory " + directory + " does not exist.");
        }
        if (recurse) {
            std::filesystem::remove_all(directory, ec);
        } else {
            std::filesystem::remove(directory, ec);
        }
        if (ec) {
            throwApp("The specified directory " + directory + " does not exist.");
        }
        return;
    }

    if (lowerAction == "rename") {
        std::string newdirectory = attrStr(attrs, "newdirectory");
        std::error_code ec;
        if (!std::filesystem::exists(directory)) {
            throwApp("The specified directory " + directory + " does not exist.");
        }
        std::filesystem::path newPath(newdirectory);
        if (!newPath.is_absolute()) {
            std::filesystem::path parent = std::filesystem::path(directory).parent_path();
            newPath = parent / newPath;
        }
        if (std::filesystem::is_directory(newPath, ec)) {
            throwApp("The new directory " + newdirectory + " already exists.");
        }
        std::filesystem::rename(directory, newPath, ec);
        if (ec) {
            throwApp("The specified directory " + directory + " cannot be renamed to " +
                     newPath.string() + ".");
        }
        return;
    }

    if (lowerAction == "copy") {
        std::string destination = attrStr(attrs, "destination");
        bool recurse = attrBool(attrs, "recurse", false);
        std::string filterStr = attrStr(attrs, "filter");
        std::error_code ec;
        if (!std::filesystem::is_directory(directory, ec)) {
            throwApp("The specified directory " + directory + " does not exist.");
        }
        std::filesystem::path dst(destination);
        if (!std::filesystem::exists(dst)) {
            std::filesystem::create_directories(dst, ec);
        }
        std::unique_ptr<ExtensionFilter> filter;
        if (!filterStr.empty() && filterStr.find_first_not_of(" \t\r\n") != std::string::npos) {
            filter.reset(new ExtensionFilter(filterStr));
        }
        std::function<void(const std::filesystem::path&, const std::filesystem::path&)> copyDir;
        copyDir = [&](const std::filesystem::path &src, const std::filesystem::path &dstd) {
            for (auto it = std::filesystem::directory_iterator(src);
                 it != std::filesystem::directory_iterator(); ++it) {
                const auto &e = *it;
                std::filesystem::path target = dstd / e.path().filename();
                if (e.is_directory()) {
                    if (recurse) {
                        std::filesystem::create_directories(target, ec);
                        copyDir(e.path(), target);
                    }
                } else if (e.is_regular_file()) {
                    if (!filter || filter->accept(e.path().filename().string())) {
                        std::filesystem::copy_file(e.path(), target,
                            std::filesystem::copy_options::overwrite_existing, ec);
                    }
                }
            }
        };
        copyDir(std::filesystem::path(directory), dst);
        return;
    }

    throwApp("Attribute validation error for CFDIRECTORY.",
             "The value of the ACTION attribute, which is currently " + action +
             ", must be one of the values: RENAME,CREATE,COPY,LIST,DELETE.");
}

} // namespace cfml
