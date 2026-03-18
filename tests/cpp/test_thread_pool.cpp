#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
#include "thread_pool.hpp"

TEST(ThreadPoolTest, ExecutesAllTasks) {
    ThreadPool pool(4);
    std::atomic<int> counter{0};
    for (int i = 0; i < 100; ++i)
        pool.enqueue([&counter] { ++counter; });
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    EXPECT_EQ(counter.load(), 100);
}

TEST(ThreadPoolTest, DestructorWaitsForTasks) {
    std::atomic<int> counter{0};
    {
        ThreadPool pool(2);
        for (int i = 0; i < 20; ++i)
            pool.enqueue([&counter] { ++counter; });
    }  // destructor blocks until all tasks done
    EXPECT_EQ(counter.load(), 20);
}
