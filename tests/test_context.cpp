#include <gtest/gtest.h>
#include <atomic>
#include <thread>
#include <chrono>

#include "internal_log.h"
#include "context/context.h"

using namespace logger::ctx;

class ContextTest : public ::testing::Test {
protected:
    void SetUp() override {
        ctx_ = Context::GetInstance();
        executor_ = ctx_->GetExecutor();
        tag_ = ctx_->CreateNewTaskRunner();
    }

    void TearDown() override {
        // 可以在这里添加清理代码
    }

    Context* ctx_;
    Executor* executor_;
    TaskRunnerTag tag_;
};

TEST_F(ContextTest, Basic) {
    std::atomic<int> counter{0};
    executor_->PostTask(tag_, [&counter]() { counter++; });

    // 等待任务执行
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    EXPECT_EQ(counter, 1);
}

TEST_F(ContextTest, PostTaskAndGetResult) {
    auto fut = executor_->PostTaskAndGetResult(tag_, []() { return 123; });
    EXPECT_EQ(fut.get(), 123);
}

TEST_F(ContextTest, DelayedTask) {
    std::atomic<int> counter{0};
    // auto start = std::chrono::steady_clock::now();

    executor_->PostDelayedTask(tag_, [&counter]() { counter++; }, std::chrono::milliseconds(100));

    // 等待足够长的时间确保延迟任务执行
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    EXPECT_EQ(counter, 1);
}

TEST_F(ContextTest, RepeatedTask) {
    std::atomic<int> counter{0};
    executor_->PostRepeatedTask(tag_, [&counter]() { counter++; }, std::chrono::milliseconds(30), 5);

    // 等待所有重复任务完成
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    EXPECT_EQ(counter, 5);
}

TEST_F(ContextTest, CancelRepeatedTask) {
    std::atomic<int> counter{0};
    auto id = executor_->PostRepeatedTask(tag_, [&counter]() { counter++; }, std::chrono::milliseconds(50), 100);

    // 让任务执行几次
    std::this_thread::sleep_for(std::chrono::milliseconds(120));

    int val_before_cancel = counter.load();
    executor_->CancelRepeatedTask(id);

    // 等待一段时间确认任务真的停止了
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    int val_after_cancel = counter.load();

    EXPECT_EQ(val_before_cancel, val_after_cancel);
    EXPECT_GE(val_before_cancel, 2);  // 至少应该执行了2次
    EXPECT_LE(val_before_cancel, 3);  // 最多执行3次
}

// 如果需要测试多个任务运行器
TEST_F(ContextTest, MultipleTaskRunners) {
    TaskRunnerTag tag1 = ctx_->CreateNewTaskRunner();
    TaskRunnerTag tag2 = ctx_->CreateNewTaskRunner();

    std::atomic<int> counter1{0};
    std::atomic<int> counter2{0};

    executor_->PostTask(tag1, [&counter1]() { counter1++; });
    executor_->PostTask(tag2, [&counter2]() { counter2++; });

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    EXPECT_EQ(counter1, 1);
    EXPECT_EQ(counter2, 1);
}