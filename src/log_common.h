#pragma once

#include <string>

#define LOGGER_LEVEL_TRACE 0
#define LOGGER_LEVEL_DEBUG 1
#define LOGGER_LEVEL_INFO 2
#define LOGGER_LEVEL_WRAN 3
#define LOGGER_LEVEL_ERROR 4
#define LOGGER_LEVEL_CRITICAL 5
#define LOGGER_LEVEL_OFF 6

namespace logger {

using StringView = std::string_view;
using MemoryBuffer = std::string;

enum class LogLevel {
    kTrace = LOGGER_LEVEL_TRACE,
    kDebug = LOGGER_LEVEL_DEBUG,
    kInfo = LOGGER_LEVEL_INFO,
    kWarn = LOGGER_LEVEL_WRAN,
    kError = LOGGER_LEVEL_ERROR,
    kCritical = LOGGER_LEVEL_CRITICAL,
    koff = LOGGER_LEVEL_OFF
};

#define LOGGER_ACTION_LEVEL LOGGER_LEVEL_TRACE

struct SourceLocation {
    constexpr SourceLocation() = default;

    SourceLocation(StringView file_name_in, int32_t line_in, StringView fun_name_in)
            : file_name{file_name_in}, line{line_in}, fun_name{fun_name_in} {
        // 提取文件名
        if (!file_name.empty()) {
            size_t pos = file_name.rfind('/');
            if (pos != StringView::npos) {
                file_name = file_name.substr(pos + 1);
            } else {
                pos = file_name.rfind('\\');
                if (pos != StringView::npos) {
                    file_name = file_name.substr(pos + 1);
                }
            }
        }
    }

    StringView file_name;
    int32_t line{0};
    StringView fun_name;
};

}  // namespace logger