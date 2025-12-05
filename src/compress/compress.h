#pragma once

#include <cstddef>
#include <string>

namespace logger {
namespace compress {
// 抽象类 选择两种不同的压缩方法
class Compression {
public:
    virtual ~Compression() = default;

    virtual size_t Compress(const void* input_data, size_t input_size, void* output_data, size_t output_size) = 0;

    virtual size_t CompressBound(size_t input_size) = 0;  // 计算压缩后的大小

    virtual std::string DeCompress(const void* input, size_t input_size) = 0;  // 解压缩

    virtual void ResetStream() = 0;  // 重置状态
};
}  // namespace compress
}  // namespace logger