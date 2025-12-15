#pragma once

#include "formatter.h"

namespace logger {
class DefaultFormatter final : public Formatter {
public:
    DefaultFormatter() = default;
    ~DefaultFormatter() = default;

    DefaultFormatter(DefaultFormatter&& other) = default;
    DefaultFormatter& operator=(DefaultFormatter&& other) = default;

    void Format(const LogMsg& msg, MemoryBuffer* dest) override;
};
}  // namespace logger