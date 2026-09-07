/**
 * @file fn_getvfsmetadata.cpp
 * @brief CFML getvfsmetadata() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>
#include <webstrada/exceptions.h>
#include <webstrada/string.h>

namespace cfml {

cfvariant *cf_getvfsmetadata(const cfvariant *fileSystemType) {
    if (!fileSystemType) {
        throw webstrada::exception("GetVFSMetaData requires exactly 1 argument");
    }
    webstrada::string fsType = const_cast<cfvariant*>(fileSystemType)->toString();
    fsType = fsType.trimmed();
    webstrada::string fsLower = fsType;
    fsLower.toLower();

    if (!fsLower.equals("ram") && !fsLower.startWith("ram:")) {
        throw webstrada::exception("FileSystemType provided is not valid.");
    }

    cfvariant st(cfvariant::Struct);
    cfvariant enabledVar(cfvariant::Boolean);
    enabledVar.m_bool = true;
    st.structSet("Enabled", enabledVar);
    st.structSet("Limit", cfvariant("20971520"));
    st.structSet("Used", cfvariant("0"));
    st.structSet("Free", cfvariant("20971520"));
    return new cfvariant(st);
}

} // namespace cfml
