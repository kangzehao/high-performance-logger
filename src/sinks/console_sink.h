#pragma once

#include "default_formatter.h"

#include <mutex>

#include "sink.h"

namespace logger {
class ConsoleSink final : public Sink {
public:
    ConsoleSink();
    ~ConsoleSink() = default;

    void Log(const LogMsg& msg) override;
    void SetFormatter(std::unique_ptr<Formatter> formatter) override;
    // void Flush() {}

private:
    std::unique_ptr<Formatter> formatter_ptr_;
    std::mutex mutex_;
};
}  // namespace logger