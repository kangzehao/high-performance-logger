#pragma once

#include "log_handle.h"

#include <fmt/core.h>

namespace logger {
class ExtensionLogHandle : public LogHandle {
public:
    // 复用基类构造函数
    using LogHandle::LogHandle;

    // 扩展功能 提供日志的fmt的参数拼接能力
    template <typename... Args>
    void Log(LogLevel level, SourceLocation loc, fmt::format_string<Args...> fmt, Args&&... args) {
        Log_(level, loc, fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void Log(LogLevel level, fmt::format_string<Args...> fmt, Args&&... args) {
        Log(SourceLocation{}, level, fmt, std::forward<Args>(args)...);
    }

private:
    template <typename... Args>
    void Log_(LogLevel level, SourceLocation loc, fmt::format_string<Args...> fmt, Args&&... args) {
        if (!ShouldLog(level)) {
            return;
        }

        // 将格式化后的内容生成到 std::string，作为消息体传递
        std::string formatted = fmt::format(fmt, std::forward<Args>(args)...);
        LogMsg msg(loc, level, StringView(formatted.data(), formatted.size()));
        LogHandle::Log_(msg);
    }
};
}  // namespace logger