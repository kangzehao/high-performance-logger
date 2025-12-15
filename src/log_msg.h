#pragma once

#include "log_common.h"

namespace logger {
struct LogMsg {
    LogMsg(SourceLocation loc, LogLevel lvl, StringView mes)
            : location(std::move(loc)), level(std::move(lvl)), message(std::move(mes)) {}
    LogMsg(LogLevel lvl, StringView mes) : LogMsg(SourceLocation{}, lvl, mes) {}

    LogMsg(const LogMsg& other) = default;
    LogMsg& operator=(const LogMsg& other) = default;

    SourceLocation location;
    LogLevel level;
    StringView message;
};
};  // namespace logger