#pragma once

#include <functional>

namespace logger {
class ExecuteAfterScopeEnd {
public:
    ExecuteAfterScopeEnd() = default;

    // 禁用拷贝移动操作
    ExecuteAfterScopeEnd(const ExecuteAfterScopeEnd& other) = delete;
    ExecuteAfterScopeEnd& operator=(const ExecuteAfterScopeEnd& other) = delete;

    ExecuteAfterScopeEnd(ExecuteAfterScopeEnd&& other) = delete;
    ExecuteAfterScopeEnd& operator=(ExecuteAfterScopeEnd&& other) = delete;

    template <typename F, typename... Args>
    ExecuteAfterScopeEnd(F&& f, Args&&... args) {
        fun_ = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
    }

    ~ExecuteAfterScopeEnd() noexcept {
        if (fun_) {
            fun_();
        }
    }

private:
    std::function<void()> fun_;
};

#define SPLICE_NAME(a, b) a##b
#define MAKE_DEFER(line) ExecuteAfterScopeEnd SPLICE_NAME(defer, line) = [&]()

#define DEFER MAKE_DEFER(__LINE__)

}  // namespace logger