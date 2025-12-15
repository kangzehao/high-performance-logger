#include "log_handle.h"
#include "sink.h"

namespace logger {
LogHandle::LogHandle(LogSinkPtrList sinks) : level_(LogLevel::kInfo) {
    for (const auto& sink : sinks) {
        if (sink) {
            sinks_.emplace_back(std::move(sink));
        }
    }
}
LogHandle::LogHandle(LogSinkPtr sink) : level_(LogLevel::kInfo) {
    if (sink) {
        sinks_.emplace_back(std::move(sink));
    }
}

void LogHandle::SetLevel(LogLevel level) {
    level_ = level;
}

LogLevel LogHandle::GetLevel() const {
    return level_;
}

void LogHandle::Log(LogLevel level, SourceLocation loc, StringView message) {
    if (!ShouldLog(level)) {
        return;
    }
    LogMsg msg(loc, level, message);
    Log_(msg);
}

void LogHandle::Log_(const LogMsg& log_msg) {
    for (const auto& sink : sinks_) {
        if (sink) {
            sink->Log(log_msg);
        }
    }
}
}  // namespace logger