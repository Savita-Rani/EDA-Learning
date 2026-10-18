#pragma once

#include <cstddef>
#include <vector>

// ============================================================================
// WaveformBuffer.h  —  Phase 9 (Frontend), the one piece of this phase
// that's actual LOGIC rather than rendering, and so the one piece that
// gets a real GoogleTest suite. See docs/phases/phase9.md, "What this
// phase does and doesn't unit-test," for why the ImGui/GLFW integration
// itself has no GoogleTest coverage (there's nothing meaningful to
// assert against a rendered frame without a human looking at it).
//
// A fixed-capacity history of samples — exactly what a waveform view
// needs: record a value every frame (or every time it changes), keep
// only the most recent N, feed that to a plotting call. Independent of
// Component/Netlist/etc. — fully generic, like Phase 7/8's
// infrastructure classes.
//
// Deliberately a std::vector with front-erase on overflow, not a
// circular buffer/std::deque — O(n) per push, but n is small (a UI
// history buffer, capacity ~100-300) and this runs once per rendered
// frame at most, nowhere near a hot path. Simpler to read and test
// correctly beats a more "efficient" design nothing here needs.
// ============================================================================

namespace eda {

class WaveformBuffer {
public:
    explicit WaveformBuffer(std::size_t capacity) : capacity_(capacity) {}

    // Appends `value`; if already at capacity, the oldest sample is
    // dropped first — the buffer always holds at most `capacity()`
    // samples, oldest-first.
    void push(float value) {
        if (samples_.size() >= capacity_) {
            samples_.erase(samples_.begin());
        }
        samples_.push_back(value);
    }

    const std::vector<float>& samples() const noexcept { return samples_; }
    std::size_t capacity() const noexcept { return capacity_; }
    std::size_t size() const noexcept { return samples_.size(); }

private:
    std::size_t capacity_;
    std::vector<float> samples_;
};

} // namespace eda
