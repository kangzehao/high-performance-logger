#pragma once

#include "formatter.h"

namespace logger {
class EffectiveFormatter final : public Formatter {
public:
    EffectiveFormatter() = default;
    ~EffectiveFormatter() = default;

    EffectiveFormatter(EffectiveFormatter&& other) = default;
    EffectiveFormatter& operator=(EffectiveFormatter&& other) = default;

    void Format(const LogMsg& msg, MemoryBuffer* dest) override;
};
}  