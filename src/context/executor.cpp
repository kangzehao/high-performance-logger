#include "executor.h"

#include "src/internal_log.h"

namespace logger {
namespace ctx {
Executor::ExecutorTimer::ExecutorTimer() {
    // 初始化
    thread_pool_ptr_ = std::make_unique<ThreadPool>(1);
    repeated_task_id_.store(0);
    is_running_.store(false);
}

Executor::ExecutorTimer::~ExecutorTimer() {
    Stop();
}

bool Executor::ExecutorTimer::Start() {
    if (is_running_.load()) {
        return false;
    }
    is_running_.store(true);
    bool ret = thread_pool_ptr_->Start();
    // 任务调度放到底层的线程池中运行
    thread_pool_ptr_->Submit([this]() { this->Run(); });
    return ret;
}

void Executor::ExecutorTimer::Run() {
    while (is_running_.load()) {
        ScheduledTask s;

        {
            std::unique_lock<std::mutex> lock(scheduled_task_queue_mutex_);
            cv_.wait(lock, [this]() { return !is_running_.load() || !scheduled_task_queue_.empty(); });

            if (!is_running_.load()) {
                break;
            }

            s = scheduled_task_queue_.top();

            // 看一下时间到了没
            auto now = std::chrono::high_resolution_clock::now();

            if (s.scheduled_time_point > now) {
                s = scheduled_task_queue_.top();
                auto time_out = s.scheduled_time_point - std::chrono::high_resolution_clock::now();
                cv_.wait_for(lock, time_out, [this, s]() {  // 被其他线程唤醒的情况下，判断是否满足时间
                    return !is_running_.load() || std::chrono::high_resolution_clock::now() >= s.scheduled_time_point;
                });
            }
            scheduled_task_queue_.pop();
        }
        s.task();
    }
}

void Executor::ExecutorTimer::Stop() {
    if (!is_running_.load()) {
        return;
    }
    is_running_.store(false);
    cv_.notify_all();
    if (thread_pool_ptr_) {
        thread_pool_ptr_.reset();
    }
}

void Executor::ExecutorTimer::PostDelayedTask(Task task, const std::chrono::microseconds& delayed_time) {
    ScheduledTask s;
    s.scheduled_time_point = delayed_time + std::chrono::high_resolution_clock::now();
    s.task = std::move(task);
    s.repeated_id = GetNextRepeatedTaskID();

    {
        std::lock_guard<std::mutex> lock(scheduled_task_queue_mutex_);
        scheduled_task_queue_.emplace(std::move(s));
    }
    cv_.notify_all();
}

RepeatedTaskID Executor::ExecutorTimer::PostRepeatedTask(Task task,
                                                         const std::chrono::microseconds& interval__time,
                                                         uint64_t repeat_num) {
    RepeatedTaskID id = GetNextRepeatedTaskID();
    {
        std::lock_guard<std::mutex> lock(repeated_task_id_set_mutex_);
        repeated_task_id_set_.insert(id);
    }
    // LOG_DEBUG("id: {}, interval__time: {}", id, interval__time);
    PostRepeatedTask(std::move(task), interval__time, id, repeat_num);
    return id;
}

// 递归添加任务，注意退出条件
void Executor::ExecutorTimer::PostRepeatedTask(Task task,
                                               const std::chrono::microseconds& interval__time,
                                               RepeatedTaskID repeated_task_id,
                                               uint64_t repeat_num) {
    {
        std::lock_guard<std::mutex> lock(repeated_task_id_set_mutex_);
        if (!repeated_task_id_set_.count(repeated_task_id) || repeat_num == 0) {
            return;
        }
    }

    task();  // 这个如果是个耗时操作, 可以放在其他工作类型调度器，定时器调度器来调度这个工作调度器件

    Task next_repeated = [this, task, interval__time, repeated_task_id, repeat_num]() {
        this->PostRepeatedTask(std::move(task), interval__time, repeated_task_id, repeat_num - 1);
    };

    ScheduledTask s;
    s.scheduled_time_point = interval__time + std::chrono::high_resolution_clock::now();
    s.task = std::move(next_repeated);
    s.repeated_id = repeated_task_id;

    {
        std::lock_guard<std::mutex> lock(scheduled_task_queue_mutex_);
        scheduled_task_queue_.emplace(std::move(s));
    }
    cv_.notify_all();
}

void Executor::ExecutorTimer::CancelRepeatedTask(RepeatedTaskID task_id) {
    std::lock_guard<std::mutex> lock(repeated_task_id_set_mutex_);
    if (!repeated_task_id_set_.count(task_id)) {
        return;
    }
    repeated_task_id_set_.erase(task_id);
}

Executor::TaskRunnerManager::TaskRunnerManager() {
    task_tag_.store(0);
}

TaskRunnerTag Executor::TaskRunnerManager::AddTaskRunner() {
    TaskRunnerTag tag = GetNextTaskRunnerTag();
    std::lock_guard<std::mutex> lock(task_runner_map_mutex_);
    while (task_runner_map_.count(tag)) {
        tag = GetNextTaskRunnerTag();
    }

    // 线程池数量为1 按顺序运行提交给线程池的任务
    TaskRunnerPtr runner = std::make_unique<TaskRunner>(1);

    runner->Start();

    // 注册到表中
    task_runner_map_.emplace(tag, std::move(runner));

    return tag;
}

Executor::TaskRunnerManager::TaskRunner* Executor::TaskRunnerManager::GetTaskRunner(const TaskRunnerTag& tag) {
    std::lock_guard<std::mutex> lock(task_runner_map_mutex_);
    if (!task_runner_map_.count(tag)) {
        return nullptr;
    }

    return task_runner_map_[tag].get();
}
Executor::Executor() {
    task_runner_manager_ = std::make_unique<TaskRunnerManager>();
    executor_timer_ = std::make_unique<ExecutorTimer>();
}

Executor::~Executor() {
    if (executor_timer_) {
        executor_timer_->Stop();
    }
    executor_timer_.reset();
    task_runner_manager_.reset();
}

TaskRunnerTag Executor::AddTaskRunner() {
    return task_runner_manager_->AddTaskRunner();
}

void Executor::PostTask(const TaskRunnerTag& runner_tag, Task task) {
    auto runner = task_runner_manager_->GetTaskRunner(runner_tag);
    if (!runner) {
        throw std::runtime_error("TaskRunner not found for tag: " + runner_tag);
    }
    runner->Submit(task);
}

}  // namespace ctx
}  // namespace logger