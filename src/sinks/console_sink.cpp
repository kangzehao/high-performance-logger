#include "console_sink.h"

namespace logger {

ConsoleSink::ConsoleSink() : formatter_ptr_(std::make_unique<DefaultFormatter>()) {}

void ConsoleSink::Log(const LogMsg& msg) {
    MemoryBuffer data;
    formatter_ptr_->Format(msg, &data);

    std::lock_guard<std::mutex> lock(mutex_);
    fwrite(data.data(), 1, data.size(), stdout);
    fwrite("\n", 1, 1, stdout);
}

void ConsoleSink::SetFormatter(std::unique_ptr<Formatter> formatter) {
    formatter_ptr_ = std::move(formatter);
}
}  // namespace logger