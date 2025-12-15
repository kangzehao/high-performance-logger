#pragma once

#include <memory>

#include "log_extension_handle.h"

namespace logger {
class LogFactory {
public:
    static LogFactory& GetInstacne();

    void SetLogHandle(std::shared_ptr<ExtensionLogHandle> handle_ptr);

    ExtensionLogHandle* GetLogHandle() const;

private:
    LogFactory() = default;

    LogFactory(const LogFactory& other) = delete;
    LogFactory& operator=(const LogFactory& other) = delete;

private:
    std::shared_ptr<ExtensionLogHandle> handle_ptr_;
};

}  // namespace logger