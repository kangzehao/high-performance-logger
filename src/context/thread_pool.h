#pragma once

#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <queue>
#include <functional>
#include <future>

namespace logger {

class ThreadPool {  // 固定线程池
public:
    ThreadPool(size_t pool_size);
    ~ThreadPool();

    // 禁用拷贝和移动操作
    ThreadPool(const ThreadPool& other) = delete;
    ThreadPool& operator=(const ThreadPool& other) = delete;

    ThreadPool(ThreadPool&& other) = delete;
    ThreadPool& operator=(ThreadPool&& other) = delete;

    bool Start();

    void Stop();

    size_t Size() const {
        return workers_vector_.capacity();
    }

    // 提交任务,获取返回值
    template <typename F, typename... Args>
    void Submit(F&& f, Args&&... argc) {
        if (!is_running_) {
            throw std::runtime_error("submit on stopped ThreadPool");
        }

        // 绑定可调用对象
        auto bound_task = std::bind(std::forward<F>(f), std::forward<Args>(argc)...);

        {
            std::unique_lock<std::mutex> lock(mutex_task_que_);
            task_que_.emplace(std::move(bound_task));
        }

        cv_.notify_one();
    }

    // 提交任务,获取返回值
    template <typename F, typename... Args>
    auto SubmitWithFuture(F&& f, Args&&... argc) -> std::future<std::invoke_result_t<F, Args...>> {
        if (!is_running_) {
            throw std::runtime_error("submit on stopped ThreadPool");
        }

        using return_type = std::invoke_result_t<F, Args...>;

        // 绑定可调用对象
        auto bound_task = std::bind(std::forward<F>(f), std::forward<Args>(argc)...);

        // std::packaged_task<return_type()> task(std::move(bound_task));

        auto share_ptr_task = std::make_shared<std::packaged_task<return_type()>>(std::move(bound_task));

        std::future<return_type> res = share_ptr_task->get_future();

        {
            std::unique_lock<std::mutex> lock(mutex_task_que_);
            // function 接受的对象需要可拷贝可移动， packaged_task 不支持拷贝，故使用lambda表达式 内部调用
            // shared_ptr进行封装
            task_que_.emplace([share_ptr_task]() { (*share_ptr_task)(); });
        }

        cv_.notify_one();

        return res;  // 返回值优化可以移动返回
    }

private:
    std::vector<std::thread> workers_vector_;

    // 类型擦除机制,存储符合void签名的可调用对象
    std::queue<std::function<void(void)>> task_que_;

    std::atomic<bool> is_running_;
    std::mutex mutex_task_que_;
    std::condition_variable cv_;

    size_t pool_size_;

private:
    void CreatePoolWorkers();

    void DeletePoolWorkers();
};

}  // namespace logger