#pragma once

#include <utility>

// ============================================================================
// OwningPtr.h  —  Phase 7 (Memory management), part 1 of 3
//
// A minimal, hand-written version of std::unique_ptr<T> — not because
// the real one needs replacing (it doesn't; use std::unique_ptr in real
// code, as this project has since Phase 2), but because writing it out
// forces every rule to be explicit instead of "the compiler probably
// does the right thing." Independent of Component/Gate/Netlist/etc. —
// fully generic, like std::unique_ptr itself. See
// docs/DECISIONS.md, "Phase 7 is independent, generic infrastructure."
//
// Concepts introduced here:
//  - RAII (Resource Acquisition Is Initialization): the constructor
//    takes ownership of a raw pointer; the destructor frees it. As long
//    as an OwningPtr exists, its resource is guaranteed held; the moment
//    it's destroyed — scope exit, exception unwinding, early return,
//    anything — the resource is guaranteed released. No path through
//    code can "forget" to clean up.
//  - The Rule of 5, written out explicitly: a class managing a resource
//    needs to decide what happens on copy, move, and destruction (5
//    special members: destructor, copy ctor, copy assignment, move ctor,
//    move assignment). Here: copying is nonsensical for exclusive
//    ownership (who would own it then?) — deleted outright. Moving
//    transfers ownership and empties the source — defined explicitly.
//  - `= delete`              : not just "unimplemented," actively
//    forbidden — attempting to copy an OwningPtr is a compile error,
//    not a linker error or runtime bug.
//  - `noexcept`              : promises these operations never throw —
//    required for move operations to be used in some standard contexts
//    (e.g. growing a std::vector<OwningPtr<T>> without falling back to
//    copying), and just generally true here (no allocation happens on
//    move, only pointer swaps).
//  - Perfect forwarding (`makeOwning`) : `Args&&...` + `std::forward`
//    pass constructor arguments through to `new T(...)` without extra
//    copies — the same mechanism std::make_unique uses internally.
//  - Converting move constructor (templated, see below) : lets
//    `OwningPtr<Base> p = makeOwning<Derived>(...);` work, so an
//    OwningPtr<Component> can directly receive ownership of a freshly
//    made AndGate/OrGate/etc.
// ============================================================================

namespace eda {

template <typename T>
class OwningPtr {
public:
    OwningPtr() noexcept = default;
    explicit OwningPtr(T* ptr) noexcept : ptr_(ptr) {}

    ~OwningPtr() { delete ptr_; }

    // Copying would mean two OwningPtrs both believe they exclusively
    // own (and will delete) the same object — a guaranteed double-free.
    // Deleted outright rather than "just don't call it."
    OwningPtr(const OwningPtr&) = delete;
    OwningPtr& operator=(const OwningPtr&) = delete;

    OwningPtr(OwningPtr&& other) noexcept : ptr_(other.ptr_) {
        other.ptr_ = nullptr; // source must no longer think it owns anything
    }

    OwningPtr& operator=(OwningPtr&& other) noexcept {
        if (this != &other) {
            delete ptr_;          // release whatever THIS already owned
            ptr_ = other.ptr_;    // take over the source's resource
            other.ptr_ = nullptr; // and empty the source
        }
        return *this;
    }

    // Converting move constructor: lets `OwningPtr<Base> p =
    // makeOwning<Derived>(...);` work, the same way real
    // std::unique_ptr does — `other.release()` yields a `U*`, which
    // only compiles here if `U*` converts to `T*` (e.g. derived-to-base
    // pointer conversion). No explicit type-trait/concept gating needed;
    // the language's own pointer-conversion rules do that for free.
    template <typename U>
    OwningPtr(OwningPtr<U>&& other) noexcept : ptr_(other.release()) {}

    T* get() const noexcept { return ptr_; }
    T* operator->() const noexcept { return ptr_; }
    T& operator*() const noexcept { return *ptr_; }
    explicit operator bool() const noexcept { return ptr_ != nullptr; }

    // Hands ownership to the CALLER — after this, the OwningPtr owns
    // nothing and will NOT delete the returned pointer.
    T* release() noexcept {
        T* released = ptr_;
        ptr_ = nullptr;
        return released;
    }

    // Replaces the owned object, deleting whatever was owned before.
    void reset(T* ptr = nullptr) noexcept {
        delete ptr_;
        ptr_ = ptr;
    }

private:
    T* ptr_ = nullptr;
};

template <typename T, typename... Args>
OwningPtr<T> makeOwning(Args&&... args) {
    return OwningPtr<T>(new T(std::forward<Args>(args)...));
}

} // namespace eda
