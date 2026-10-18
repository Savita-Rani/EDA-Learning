#pragma once

#include <cstdio>
#include <string>

// ============================================================================
// FileHandle.h  —  Phase 7 (Memory management), part 3 of 3
//
// RAII isn't only for heap memory (OwningPtr.h/SharedOwningPtr.h) — ANY
// resource that must be acquired and released follows the same shape.
// FileHandle wraps a C `FILE*` (acquired via fopen, released via
// fclose) specifically to demonstrate that the Rule of 5 applies
// identically to a completely different kind of resource: deleted copy
// (two FileHandles can't both own and close the same OS file handle),
// defined move (transfers ownership of the open file).
//
// Not a template — FILE* isn't generic the way T* is, so this is an
// ordinary class with a normal .cpp, unlike OwningPtr/SharedOwningPtr.
// ============================================================================

namespace eda {

class FileHandle {
public:
    // Opens `path` in `mode` (fopen's mode string — "w" to write/
    // truncate, "a" to append, ...). `isOpen()` reports whether it
    // actually succeeded; this constructor does not throw on failure,
    // by design — callers check isOpen() rather than catching.
    explicit FileHandle(const std::string& path, const char* mode = "w");

    ~FileHandle();

    FileHandle(const FileHandle&) = delete;
    FileHandle& operator=(const FileHandle&) = delete;

    FileHandle(FileHandle&& other) noexcept;
    FileHandle& operator=(FileHandle&& other) noexcept;

    bool isOpen() const noexcept { return file_ != nullptr; }

    // Writes `line` followed by a newline. A no-op (not a crash, not
    // undefined behavior) if the handle isn't open — including after
    // being moved from.
    void writeLine(const std::string& line);

private:
    FILE* file_ = nullptr;
};

} // namespace eda
