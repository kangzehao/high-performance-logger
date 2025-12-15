#include <memory>
#include "logger.h"
#include "sinks/console_sink.h"
#include "log_extension_handle.h"

int main() {
    // 创建一个控制台输出的 sink
    auto console_sink = std::make_shared<logger::ConsoleSink>();

    // 创建扩展日志句柄，并设置日志级别为 Info
    auto handle = std::make_shared<logger::ExtensionLogHandle>(console_sink);
    handle->SetLevel(logger::LogLevel::kInfo);

    // 注册到全局工厂，后续宏 EXT_LOG_* 将使用这里的 handle
    logger::LogFactory::GetInstacne().SetLogHandle(handle);

    // 直接使用宏输出不同级别的日志
    EXT_LOG_INFO("hello {}", "logger");
    EXT_LOG_WARN("something might be wrong, code={} ", 42);
    EXT_LOG_ERROR("an error occurred: {}", "network timeout");

    // 也可以直接使用句柄调用，传入 SourceLocation
    handle->Log(logger::LogLevel::kInfo,
                logger::SourceLocation{__FILE__, __LINE__, __FUNCTION__},
                "direct call without macro");

    return 0;
}
