#pragma once

#include <memory>

#include "log_msg.h"
#include "formatter.h"

namespace logger {
class Sink {
public:
    Sink() = default;
    virtual ~Sink() = default;

    virtual void Log(const LogMsg& msg) = 0;
    virtual void SetFormatter(std::unique_ptr<Formatter> formatter) = 0;
    virtual void Flush() {}
};
}  // namespace logger