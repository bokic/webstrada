/**
 * @file fn_directorylist.cpp
 * @brief CFML directorylist() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>
#include <webstrada/exceptions.h>
#include <webstrada/string.h>
#include <webstrada/upload.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cctype>
#include <cstring>
#include <ctime>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

using webstrada::UploadedFile;

namespace cfml {

cfvariant *cf_directorylist(const cfvariant *path, const cfvariant *recurse, const cfvariant *filter, const cfvariant *sort, const cfvariant *type) {
    if (!path) throw webstrada::exception("DirectoryList: Missing argument");
    string pStr = const_cast<cfvariant*>(path)->toString();
    bool rec = recurse ? isTruthy(*recurse) : false;

    string filterStr;
    if (filter) filterStr = const_cast<cfvariant*>(filter)->toString();

    string sortStr;
    if (sort) sortStr = const_cast<cfvariant*>(sort)->toString();
    sortStr.toUpper();

    string typeStr;
    if (type) typeStr = const_cast<cfvariant*>(type)->toString();
    typeStr.toUpper();

    auto *ret = new cfvariant(cfvariant::Array);

    if (!std::filesystem::exists(pStr.constData()) || !std::filesystem::is_directory(pStr.constData())) {
        return ret;
    }

    auto addEntry = [&](const std::filesystem::directory_entry &entry) {
        std::filesystem::path fp = entry.path();
        string name = fp.filename().string().c_str();
        string fullPath = fp.string().c_str();

        bool isDir = entry.is_directory();
        bool isFile = entry.is_regular_file();

        if (!typeStr.isEmpty()) {
            if (typeStr.equals("DIR") && !isDir) return;
            if (typeStr.equals("FILE") && !isFile) return;
        }

        // Apply name filter (simple wildcard: * and ?)
        if (!filterStr.isEmpty()) {
            string f = filterStr;
            bool match = false;
            if (f.at(0) == '*' && f.at(f.length() - 1) == '*') {
                string mid = f.mid(1, f.length() - 2);
                match = name.contains(mid);
            } else if (f.at(0) == '*') {
                match = name.endsWith(f.mid(1, f.length() - 1));
            } else if (f.at(f.length() - 1) == '*') {
                match = name.startWith(f.left(f.length() - 1).constData());
            } else {
                match = name.equals(f);
            }
            if (!match) return;
        }

        cfvariant entryVar(cfvariant::Struct);
        entryVar.set("NAME").set_type(cfvariant::String);
        entryVar.set("NAME").m_str->append(name);
        entryVar.set("PATH").set_type(cfvariant::String);
        entryVar.set("PATH").m_str->append(fullPath);

        if (isDir) {
            entryVar.set("SIZE").set_type(cfvariant::Number);
            entryVar.set("SIZE").m_int = 0;
            entryVar.set("TYPE").set_type(cfvariant::String);
            entryVar.set("TYPE").m_str->append("dir");
        } else if (isFile) {
            entryVar.set("SIZE").set_type(cfvariant::Number);
            entryVar.set("SIZE").m_int = static_cast<int>(entry.file_size());
            entryVar.set("TYPE").set_type(cfvariant::String);
            entryVar.set("TYPE").m_str->append("file");
        } else {
            entryVar.set("SIZE").set_type(cfvariant::Number);
            entryVar.set("SIZE").m_int = 0;
            entryVar.set("TYPE").set_type(cfvariant::String);
            entryVar.set("TYPE").m_str->append("other");
        }

        ret->m_array->push_back(entryVar);
    };

    if (rec) {
        for (auto it = std::filesystem::recursive_directory_iterator(
                 pStr.constData(),
                 std::filesystem::directory_options::skip_permission_denied);
             it != std::filesystem::recursive_directory_iterator(); ++it) {
            addEntry(*it);
        }
    } else {
        for (auto it = std::filesystem::directory_iterator(pStr.constData());
             it != std::filesystem::directory_iterator(); ++it) {
            addEntry(*it);
        }
    }

    return ret;
}

} // namespace cfml
