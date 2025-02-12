#include "thread_pool.hpp"
#include "timers.hpp"
#include <gtest/gtest.h>

int Fib(int in)
{
    int out = 1;
    for (int i = 1; i <= in; i++)
    {
        out *= i;
    }
    return out;
}

struct FibTask
{
    int in;
    int operator()() const { return Fib(in); }
};

struct WaitTask
{
    int milliseconds = 1;
    void operator()() const { std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds)); }
};

TEST(ThreadPoolTests, QueueAndExecute)
{
    ThreadPool pool { 1 };
    pool.Start();

    auto future = pool.QueueWork(FibTask(10));
    EXPECT_EQ(future.get(), 3'628'800);
}

TEST(ThreadPoolTests, WaitForAllTasks)
{
    ThreadPool pool { 1 };
    auto future = pool.QueueWork(FibTask(100));

    pool.Start();
    pool.FinishPendingWork();

    // Check if the future is already ready
    EXPECT_NE(future.wait_for(std::chrono::milliseconds(0)), std::future_status::timeout);
}

TEST(ThreadPoolTests, TaskCancelled)
{
    ThreadPool pool { 1 };
    auto future = pool.QueueWork(FibTask(100));
    pool.CancelAll();

    EXPECT_THROW(future.get(), std::future_error);
}

TEST(ThreadPoolTests, TaskParallelism)
{
    constexpr int WAIT_TIME = 10;
    uint32_t THREAD_COUNT = 6;

    ThreadPool pool { THREAD_COUNT };

    for (uint32_t i = 0; i < THREAD_COUNT; i++)
    {
        pool.QueueWork(WaitTask(WAIT_TIME));
    }

    Stopwatch t {};
    pool.Start();
    pool.FinishPendingWork();
    auto elapsed = t.GetElapsed().count();

    EXPECT_LT(elapsed, WAIT_TIME * THREAD_COUNT);
}