#include <gtest/gtest.h>
#include <vector>
#include <atomic>
#include <thread>
#include <chrono>
#include <future>

#include "internal_log.h"
#include "context/thread_pool.h"

using namespace logger;

class ThreadPoolTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 可以在这里添加每个测试前的设置代码
    }

    void TearDown() override {
        // 可以在这里添加每个测试后的清理代码
    }

    // 等待所有任务完成（用于无返回值的任务）
    void WaitForCompletion(ThreadPool& pool,
                           int expected_count,
                           std::atomic<int>& counter,
                           std::chrono::milliseconds timeout = std::chrono::milliseconds(500)) {
        auto start = std::chrono::steady_clock::now();
        while (counter.load() < expected_count && std::chrono::steady_clock::now() - start < timeout) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
};

TEST_F(ThreadPoolTest, SimpleTask) {
    ThreadPool pool(2);
    pool.Start();

    auto fut = pool.SubmitWithFuture([]() { return 42; });

    EXPECT_EQ(fut.get(), 42);
}

TEST_F(ThreadPoolTest, MultipleTasks) {
    ThreadPool pool(4);
    pool.Start();
    std::vector<std::future<int>> results;

    // 提交多个任务
    for (int i = 0; i < 10; ++i) {
        results.push_back(pool.SubmitWithFuture([i]() { return i * i; }));
    }

    // 验证所有结果
    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(results[i].get(), i * i);
    }
}

TEST_F(ThreadPoolTest, ParallelIncrement) {
    ThreadPool pool(4);
    pool.Start();
    std::atomic<int> counter{0};
    std::vector<std::future<void>> tasks;

    // 提交100个增量任务
    for (int i = 0; i < 100; ++i) {
        tasks.push_back(pool.SubmitWithFuture([&counter]() { counter++; }));
    }

    // 等待所有任务完成
    for (auto& f : tasks) {
        f.get();
    }

    EXPECT_EQ(counter, 100);
}

TEST_F(ThreadPoolTest, SubmitVoidTask) {
    ThreadPool pool(2);
    pool.Start();
    std::atomic<int> counter{0};

    // 提交10个无返回值的任务
    for (int i = 0; i < 10; ++i) {
        pool.Submit([&counter]() { counter++; });
    }

    // 等待所有任务执行完毕
    WaitForCompletion(pool, 10, counter);

    EXPECT_EQ(counter, 10);
}

TEST_F(ThreadPoolTest, DifferentThreadPoolSizes) {
    // 测试不同大小的线程池
    std::vector<size_t> pool_sizes = {1, 2, 4, 8};

    for (size_t size : pool_sizes) {
        ThreadPool pool(size);
        pool.Start();
        std::atomic<int> counter{0};
        std::vector<std::future<void>> tasks;

        // 提交比线程池大小更多的任务
        for (int i = 0; i < static_cast<int>(size * 2); ++i) {
            tasks.push_back(pool.SubmitWithFuture([&counter]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                counter++;
            }));
        }

        // 等待所有任务完成
        for (auto& f : tasks) {
            f.get();
        }

        EXPECT_EQ(counter, size * 2) << "Failed with pool size: " << size;
    }
}

TEST_F(ThreadPoolTest, ExceptionHandling) {
    ThreadPool pool(2);
    pool.Start();

    // 测试抛出异常的任务
    auto fut = pool.SubmitWithFuture([]() -> int {
        throw std::runtime_error("Test exception");
        return 42;
    });

    EXPECT_THROW(fut.get(), std::runtime_error);
}

TEST_F(ThreadPoolTest, TaskOrdering) {
    ThreadPool pool(1);  // 单线程，任务应该按顺序执行
    pool.Start();
    std::vector<int> execution_order;
    std::mutex order_mutex;

    std::vector<std::future<void>> tasks;
    for (int i = 0; i < 5; ++i) {
        tasks.push_back(pool.SubmitWithFuture([i, &execution_order, &order_mutex]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            std::lock_guard<std::mutex> lock(order_mutex);
            execution_order.push_back(i);
        }));
    }

    // 等待所有任务完成
    for (auto& f : tasks) {
        f.get();
    }

    // 验证执行顺序（单线程池应该保持提交顺序）
    EXPECT_EQ(execution_order.size(), 5);
    for (int i = 0; i < 5; ++i) {
        EXPECT_EQ(execution_order[i], i);
    }
}

TEST_F(ThreadPoolTest, RestartPool) {
    ThreadPool pool(2);

    // 第一次启动
    pool.Start();
    auto fut1 = pool.SubmitWithFuture([]() { return 1; });
    EXPECT_EQ(fut1.get(), 1);

    // 停止后重新启动
    pool.Stop();  // 假设 ThreadPool 有 Stop 方法
    pool.Start();

    auto fut2 = pool.SubmitWithFuture([]() { return 2; });
    EXPECT_EQ(fut2.get(), 2);
}

TEST_F(ThreadPoolTest, HeavyLoad) {
    ThreadPool pool(std::thread::hardware_concurrency());
    pool.Start();
    std::atomic<int> counter{0};
    std::vector<std::future<void>> tasks;

    // 提交大量任务
    const int task_count = 1000;
    for (int i = 0; i < task_count; ++i) {
        tasks.push_back(pool.SubmitWithFuture([&counter]() {
            // 模拟一些工作
            std::this_thread::sleep_for(std::chrono::microseconds(100));
            counter++;
        }));
    }

    // 等待所有任务完成
    for (auto& f : tasks) {
        f.get();
    }

    EXPECT_EQ(counter, task_count);
}

TEST_F(ThreadPoolTest, MoveOnlyTypes) {
    ThreadPool pool(2);
    pool.Start();

    // 测试移动语义
    auto unique_task = []() -> std::unique_ptr<int> { return std::make_unique<int>(123); };

    auto fut = pool.SubmitWithFuture(unique_task);
    auto result = fut.get();

    EXPECT_NE(result, nullptr);
    EXPECT_EQ(*result, 123);
}

// 性能测试（可选，使用 DISABLED_ 前缀可以暂时禁用）
TEST_F(ThreadPoolTest, DISABLED_PerformanceComparison) {
    const int num_tasks = 10000;
    const int task_complexity = 1000;

    // 测试单线程执行时间
    auto start_single = std::chrono::high_resolution_clock::now();
    std::atomic<int> single_counter{0};
    for (int i = 0; i < num_tasks; ++i) {
        // 模拟计算工作
        int result = 0;
        for (int j = 0; j < task_complexity; ++j) {
            result += j * j;
        }
        single_counter += result;
    }
    auto end_single = std::chrono::high_resolution_clock::now();
    auto single_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_single - start_single);

    // 测试线程池执行时间
    ThreadPool pool(std::thread::hardware_concurrency());
    pool.Start();
    auto start_pool = std::chrono::high_resolution_clock::now();
    std::atomic<int> pool_counter{0};
    std::vector<std::future<void>> tasks;

    for (int i = 0; i < num_tasks; ++i) {
        tasks.push_back(pool.SubmitWithFuture([&pool_counter, task_complexity]() {
            int result = 0;
            for (int j = 0; j < task_complexity; ++j) {
                result += j * j;
            }
            pool_counter += result;
        }));
    }

    for (auto& fut : tasks) {
        fut.get();
    }
    auto end_pool = std::chrono::high_resolution_clock::now();
    auto pool_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_pool - start_pool);

    LOG_INFO("Single thread time: {} ms", single_duration.count());
    LOG_INFO("Thread pool time: {} ms", pool_duration.count());
    LOG_INFO("Speedup: {:.2f}x", static_cast<double>(single_duration.count()) / pool_duration.count());

    // 验证两种方式结果一致
    EXPECT_EQ(single_counter.load(), pool_counter.load());

    // 性能断言：线程池应该比单线程快
    EXPECT_LT(pool_duration.count(), single_duration.count());
}