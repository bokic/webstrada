#include <webstrada/upload.h>

namespace webstrada {

thread_local std::vector<UploadedFile> UploadRegistry::s_files;

UploadRegistry &UploadRegistry::instance() {
    static UploadRegistry reg;
    return reg;
}

void UploadRegistry::setFiles(std::vector<UploadedFile> &&files) {
    s_files = std::move(files);
}

const std::vector<UploadedFile> &UploadRegistry::files() const {
    return s_files;
}

}
