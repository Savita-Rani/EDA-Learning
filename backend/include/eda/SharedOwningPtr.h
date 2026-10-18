#pragma once

#include <cstddef>
#include <utility>

// ============================================================================
// SharedOwningPtr.h  —  Phase 7 (Memory management), part 2 of 3
//
// A minimal, hand-written version of std::shared_ptr<T> — SHARED
// ownership, where multiple SharedOwningPtrs can point at the same
// object, and it's only deleted once the LAST one goes away. Deliberately
// uses a different idiom than OwningPtr.h for its assignment operator
// (copy-and-swap, below) — two valid ways to satisfy the Rule of 5
// correctly, for contrast.
//
// Concepts introduced here:
//  - Reference counting: a small heap-allocated `std::size_t` ("the
//    control block," radically simplified here — real std::shared_ptr's
//    control block also stores the deleter and does atomic increments
//    for thread safety) shared by every SharedOwningPtr pointing at the
//    same object. Copying increments it; destruction decrements it;
//    the object is only deleted when the count reaches zero.
//  - Copy-and-swap idiom: `operator=(SharedOwningPtr other)` takes its
//    argument BY VALUE, not by reference. That parameter is constructed
//    via the copy constructor (for `a = b`) or the move constructor
//    (for `a = std::move(b)`) automatically, depending on what's passed
//    — one function body handles both copy-assignment and
//    move-assignment correctly, and `swap` makes it exception-safe (if
//    constructing `other` throws, `*this` was never touched).
//  - Move vs copy, contrasted directly: copying increments the shared
//    count (two owners now); moving does NOT (ownership transfers,
//    still exactly one "slot," just relocated) — see useCount() in the
//    tests for this distinction made concrete.
// ============================================================================

namespace eda {

template <typename T>
class SharedOwningPtr {
public:
    SharedOwningPtr() noexcept = default;

    explicit SharedOwningPtr(T* ptr)
        : ptr_(ptr), count_(ptr ? new std::size_t(1) : nullptr) {}

    SharedOwningPtr(const SharedOwningPtr& other) noexcept
        : ptr_(other.ptr_), count_(other.count_) {
        if (count_) {
            ++(*count_);
        }
    }

    SharedOwningPtr(SharedOwningPtr&& other) noexcept
        : ptr_(other.ptr_), count_(other.count_) {
        other.ptr_ = nullptr;
        other.count_ = nullptr; // moved-from: owns nothing, count untouched
    }

    // One function, by-value parameter, handles copy-assign AND
    // move-assign — see the comment block above.
    SharedOwningPtr& operator=(SharedOwningPtr other) noexcept {
        swap(other);
        return *this; // `other` (holding what *this used to own) is
                       // destroyed when it goes out of scope here.
    }

    ~SharedOwningPtr() { release(); }

    void swap(SharedOwningPtr& other) noexcept {
        std::swap(ptr_, other.ptr_);
        std::swap(count_, other.count_);
    }

    std::size_t useCount() const noexcept { return count_ ? *count_ : 0; }
    T* get() const noexcept { return ptr_; }
    T* operator->() const noexcept { return ptr_; }
    T& operator*() const noexcept { return *ptr_; }
    explicit operator bool() const noexcept { return ptr_ != nullptr; }

private:
    void release() noexcept {
        if (count_ && --(*count_) == 0) {
            delete ptr_;
            delete count_;
        }
        ptr_ = nullptr;
        count_ = nullptr;
    }

    T* ptr_ = nullptr;
    std::size_t* count_ = nullptr;
};

template <typename T, typename... Args>
SharedOwningPtr<T> makeShared(Args&&... args) {
    return SharedOwningPtr<T>(new T(std::forward<Args>(args)...));
}

} // namespace eda
