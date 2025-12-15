#include "log_factory.h"

namespace logger {

LogFactory& LogFactory::GetInstacne() {
    static LogFactory instance;
    return instance;
}

void LogFactory::SetLogHandle(std::shared_ptr<ExtensionLogHandle> handle_ptr) {
    handle_ptr_ = handle_ptr;
};

ExtensionLogHandle* LogFactory::GetLogHandle() const {
    return handle_ptr_.get();
}

};  // namespace logger