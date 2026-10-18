#include "eda/ThreadPool.h"

// ============================================================================
// ThreadPool.cpp — Phase 8 implementation, part 1.
//
// Why pendingTasks_'s decrement AND the allTasksDone_ notify both
// happen while still holding `mutex_`, instead of unlocking first for
// better throughput: a classic "lost wakeup" race.
//
//   If the decrement+check+notify happened WITHOUT the lock:
//     1. waitUntilIdle() locks mutex_, checks pendingTasks_ == 0 -> false
//        (still 1), is ABOUT to call condition_variable::wait()...
//     2. ...but a worker finishes its task right then, decrements
//        pendingTasks_ to 0, and calls notify_all() — WITHOUT holding
//        the mutex, so nothing stops this from happening between steps
//        1 and 3.
//     3. waitUntilIdle() now actually calls wait() and blocks — but the
//        ONE notification that would have woken it already fired and
//        was lost (nobody was listening yet). Permanent hang.
//
// Keeping the decrement+notify under the SAME mutex waitUntilIdle()
// uses for its predicate check closes this gap: the two operations
// ("check predicate" and "change state that the predicate depends on")
// can never interleave. This trades a little throughput (workers hold
// the lock slightly longer) for being obviously, provably correct —
// the right tradeoff for this project.
// ============================================================================

namespace eda {

ThreadPool::ThreadPool(std::size_t numThreads) {
    workers_.reserve(numThreads);
    for (std::size_t i = 0; i < numThreads; ++i) {
        workers_.emplace_back([this] { workerLoop(); });
    }
}

ThreadPool::~ThreadPool() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stop_ = true;
    }
    taskAvailable_.notify_all(); // wake every worker so it can see stop_ and exit
    for (std::thread& worker : workers_) {
        worker.join();
    }
}

void ThreadPool::submit(std::function<void()> task) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        tasks_.push(std::move(task));
        ++pendingTasks_;
    }
    taskAvailable_.notify_one();
}

void ThreadPool::waitUntilIdle() {
    std::unique_lock<std::mutex> lock(mutex_);
    allTasksDone_.wait(lock, [this] { return pendingTasks_ == 0; });
}

void ThreadPool::workerLoop() {
    while (true) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            taskAvailable_.wait(lock, [this] { return stop_ || !tasks_.empty(); });
            if (stop_ && tasks_.empty()) {
                return;
            }
            task = std::move(tasks_.front());
            tasks_.pop();
        }

        task(); // run OUTSIDE the lock — this is the actual parallelism

        {
            std::lock_guard<std::mutex> lock(mutex_);
            --pendingTasks_;
            if (pendingTasks_ == 0) {
                allTasksDone_.notify_all();
            }
        }
    }
}

} // namespace eda
