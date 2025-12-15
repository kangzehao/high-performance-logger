#pragma once

#include "log_common.h"
#include "log_msg.h"

namespace logger {
class Formatter {
public:
    virtual ~Formatter() = default;

    virtual void Format(const LogMsg& src, MemoryBuffer* dest) = 0;
};
}  // namespace logger