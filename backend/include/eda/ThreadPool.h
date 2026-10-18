#pragma once

#include <condition_variable>
#include <cstddef>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

// ============================================================================
// ThreadPool.h  —  Phase 8 (Concurrency), part 1 of 2
//
// Fully generic, independent infrastructure — no dependency on
// Component/Gate/etc., same category as Phase 7's OwningPtr/
// SharedOwningPtr/FileHandle. See docs/DECISIONS.md, "Phase 8's
// ThreadPool is independent; ParallelSimulator depends on it + Component."
//
// A fixed set of worker threads, each pulling tasks off one shared
// queue. submit() hands off a task; waitUntilIdle() blocks until every
// submitted task (queued or currently running) has finished — the
// "barrier" ParallelSimulator needs between dependency levels.
//
// Concepts introduced here:
//  - std::thread            : one OS thread per worker, started in the
//    constructor, joined in the destructor (RAII — same promise as
//    Phase 7's classes: however this object's scope ends, cleanup runs).
//  - std::mutex + std::condition_variable : the standard
//    producer/consumer pattern. ALL shared-state reads/writes
//    (`tasks_`, `pendingTasks_`, `stop_`) happen under `mutex_` —
//    deliberately not optimized to avoid locking on the notify side,
//    specifically to avoid a classic "lost wakeup" bug (see
//    ThreadPool.cpp's comment for the exact race this sidesteps).
//  - A lambda capturing `this`  : `[this] { workerLoop(); }` — how each
//    std::thread is told what to run.
// ============================================================================

namespace eda {

class ThreadPool {
public:
    explicit ThreadPool(std::size_t numThreads);
    ~ThreadPool();

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    // Queues `task` to run on whichever worker becomes free next.
    void submit(std::function<void()> task);

    // Blocks until every task submitted so far (queued or in-flight)
    // has completed. Safe to call repeatedly — each call establishes a
    // fresh barrier for whatever's been submitted up to that point.
    void waitUntilIdle();

private:
    void workerLoop();

    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex mutex_;
    std::condition_variable taskAvailable_;
    std::condition_variable allTasksDone_;
    std::size_t pendingTasks_ = 0; // queued + currently executing
    bool stop_ = false;
};

} // namespace eda
