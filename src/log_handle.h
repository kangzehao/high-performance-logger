#pragma once

#include <atomic>
#include <memory>
#include <vector>

#include "log_msg.h"

namespace logger {

class Sink;
// 可能有多个日志实例写向同一个sink
using LogSinkPtr = std::shared_ptr<Sink>;

// 可能会有一个日志实例写向多个sink
using LogSinkPtrList = std::initializer_list<LogSinkPtr>;

class LogHandle {
public:
    explicit LogHandle(LogSinkPtrList sinks);

    explicit LogHandle(LogSinkPtr sink);

    template <typename It>
    LogHandle(It begin, It end) {
        sinks_ = std::vector<LogSinkPtr>(begin, end);
    }

    ~LogHandle() = default;

    LogHandle(const LogHandle& other) = delete;
    LogHandle& operator=(const LogHandle& other) = delete;

    void SetLevel(LogLevel level);

    LogLevel GetLevel() const;

    void Log(LogLevel level, SourceLocation loc, StringView message);

protected:
    bool ShouldLog(LogLevel level) const noexcept {
        return level >= level_;
    }

    void Log_(const LogMsg& log_msg);

private:
    std::atomic<LogLevel> level_;
    std::vector<LogSinkPtr> sinks_;
};
}  // namespace logger