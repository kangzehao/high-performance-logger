#pragma once

#include "executor.h"

namespace logger {
namespace ctx {

class ExecutorManager;

class Context {
public:
    ~Context();
    // 禁用拷贝/移动
    Context(const Context& other) = delete;
    Context& operator=(const Context& other) = delete;

    Context(Context&& other) = delete;
    Context& operator=(Context&& other) = delete;

    static Context* GetInstance() {
        static Context* instance = new Context();
        return instance;
    }
    Executor* GetExecutor() const;
    TaskRunnerTag CreateNewTaskRunner() const;

private:
    Context();

private:
    std::unique_ptr<ExecutorManager> executor_manager_;
};
}  // namespace ctx
}  // namespace logger

#define CONTEXT logger::ctx::Context::GetInstance()

#define EXECUTOR CONTEXT->GetExecutor()

#define CREATE_NEW_TASK_RUNNER CONTEXT->CreateNewTaskRunner()

#define POST_TASK(runner_tag, task) EXECUTOR->PostTask(runner_tag, task)

#define WAIT_TASK_IDLE(runner_tag) EXECUTOR->PostTaskAndGetResult(runner_tag, []() {}).wait()

#define POST_REPEATED_TASK(runner_tag, task, interval__time, repeat_num) \
    EXECUTOR->PostRepeatedTask(runner_tag, task, interval__time, repeat_num)
