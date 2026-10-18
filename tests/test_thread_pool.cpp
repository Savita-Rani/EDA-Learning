#include <gtest/gtest.h>
#include <atomic>
#include <eda/ThreadPool.h>

// ============================================================================
// test_thread_pool.cpp — Phase 8 tests, part 1.
//
// No timing-based assertions here (e.g. "parallel was faster than
// serial") — that's the demo's job, and timing assertions in a test
// suite are a classic source of flaky CI. These tests check pure
// correctness: did every submitted task actually run, exactly once,
// and does the pool remain usable across multiple submit/wait rounds.
// ============================================================================

using namespace eda;

TEST(ThreadPoolTest, AllSubmittedTasksRun) {
    ThreadPool pool(4);
    std::atomic<int> counter{0};

    constexpr int kTaskCount = 100;
    for (int i = 0; i < kTaskCount; ++i) {
        pool.submit([&counter] { counter.fetch_add(1, std::memory_order_relaxed); });
    }
    pool.waitUntilIdle();

    EXPECT_EQ(counter.load(), kTaskCount);
}

TEST(ThreadPoolTest, WaitUntilIdleWithNoTasksReturnsImmediately) {
    ThreadPool pool(2);
    pool.waitUntilIdle(); // must not hang
    SUCCEED();
}

TEST(ThreadPoolTest, MultipleRoundsOfSubmitAndWaitAreIndependentBarriers) {
    ThreadPool pool(4);
    std::atomic<int> counter{0};

    for (int i = 0; i < 10; ++i) {
        pool.submit([&counter] { counter.fetch_add(1, std::memory_order_relaxed); });
    }
    pool.waitUntilIdle();
    EXPECT_EQ(counter.load(), 10);

    for (int i = 0; i < 15; ++i) {
        pool.submit([&counter] { counter.fetch_add(1, std::memory_order_relaxed); });
    }
    pool.waitUntilIdle();
    EXPECT_EQ(counter.load(), 25); // cumulative — same counter, second round added to it
}

TEST(ThreadPoolTest, SingleWorkerThreadStillRunsAllTasks) {
    ThreadPool pool(1);
    std::atomic<int> counter{0};

    for (int i = 0; i < 20; ++i) {
        pool.submit([&counter] { counter.fetch_add(1, std::memory_order_relaxed); });
    }
    pool.waitUntilIdle();

    EXPECT_EQ(counter.load(), 20);
}

TEST(ThreadPoolTest, DestructorDrainsRemainingQueuedTasksBeforeStopping) {
    std::atomic<int> counter{0};
    {
        ThreadPool pool(4);
        for (int i = 0; i < 50; ++i) {
            pool.submit([&counter] { counter.fetch_add(1, std::memory_order_relaxed); });
        }
        // Deliberately NO waitUntilIdle() here — the destructor (at the
        // end of this scope) must still finish every queued task before
        // joining its worker threads, not abandon them.
    }

    EXPECT_EQ(counter.load(), 50);
}
