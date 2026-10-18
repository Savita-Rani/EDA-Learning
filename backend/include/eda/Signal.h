#pragma once

#include <algorithm>
#include <cstddef>
#include <functional>
#include <utility>
#include <vector>

// ============================================================================
// Signal.h  —  Phase 10 (Design patterns), part 3 of 3
//
// Phase 4's EventSimulator already has an Observer implementation —
// addWatcher(wireName, std::function<void(bool)>) — but it's baked
// directly into EventSimulator specifically for "a wire's bool value
// changed." Signal<Args...> is what that pattern looks like REFINED
// into a standalone, fully reusable primitive: connect/emit works for
// ANY event shape (zero args, one arg of any type, several args of
// mixed types), completely decoupled from wires/circuits/EventSimulator
// entirely. Fully independent, generic infrastructure — same category
// as Phase 7/8's classes, and header-only for the same reason Phase
// 6/7's templates were (a template needs its definition visible at
// every instantiation site).
//
// Concepts introduced here:
//  - Variadic templates (`template <typename... Args>`): a template
//    parameterized over an arbitrary-length, arbitrary-type parameter
//    list — `Signal<bool>`, `Signal<int, int>`, `Signal<std::string>`
//    all come from this ONE class definition.
//  - `std::function<void(Args...)>` as the slot type, mirroring the
//    pack directly into the callback signature.
//  - connect() returns an opaque ID so a specific slot can later be
//    disconnect()-ed — necessary because std::function has no identity
//    to compare against (two functions "doing the same thing" aren't
//    necessarily ==-comparable, and std::function doesn't support ==
//    at all against another std::function).
// ============================================================================

namespace eda {

template <typename... Args>
class Signal {
public:
    using Slot = std::function<void(Args...)>;

    std::size_t connect(Slot slot) {
        std::size_t id = nextId_++;
        slots_.emplace_back(id, std::move(slot));
        return id;
    }

    void disconnect(std::size_t id) {
        slots_.erase(
            std::remove_if(slots_.begin(), slots_.end(),
                            [id](const auto& entry) { return entry.first == id; }),
            slots_.end());
    }

    void emit(Args... args) const {
        for (const auto& entry : slots_) {
            entry.second(args...);
        }
    }

    std::size_t slotCount() const noexcept { return slots_.size(); }

private:
    std::vector<std::pair<std::size_t, Slot>> slots_;
    std::size_t nextId_ = 0;
};

} // namespace eda
