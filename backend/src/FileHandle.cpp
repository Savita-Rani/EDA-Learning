#include "eda/FileHandle.h"

namespace eda {

FileHandle::FileHandle(const std::string& path, const char* mode) {
    file_ = std::fopen(path.c_str(), mode);
}

FileHandle::~FileHandle() {
    if (file_) {
        std::fclose(file_); // flushes and closes — the RAII guarantee:
                             // this runs no matter how the destructor's
                             // caller got here (normal scope exit, early
                             // return, exception unwinding).
    }
}

FileHandle::FileHandle(FileHandle&& other) noexcept : file_(other.file_) {
    other.file_ = nullptr;
}

FileHandle& FileHandle::operator=(FileHandle&& other) noexcept {
    if (this != &other) {
        if (file_) {
            std::fclose(file_);
        }
        file_ = other.file_;
        other.file_ = nullptr;
    }
    return *this;
}

void FileHandle::writeLine(const std::string& line) {
    if (!file_) {
        return;
    }
    std::fputs(line.c_str(), file_);
    std::fputc('\n', file_);
}

} // namespace eda
