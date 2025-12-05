#pragma once

#include <memory>
#include <zlib.h>

#include "compress.h"

namespace logger {
namespace compress {

// 自定义删除器
struct ZStreamDeflateDeleter {
    void operator()(z_stream* stream) {
        if (stream) {
            deflateEnd(stream);  // 清理 zlib 压缩流内部资源 delete stream;
            delete stream;
            stream = nullptr;
        }
    }
};

struct ZStreamInflateDeleter {
    void operator()(z_stream* stream) {
        if (stream) {
            inflateEnd(stream);  // 清理 zlib 解压流内部资源 delete stream;
            delete stream;
            stream = nullptr;
        }
    }
};

class ZlibCompress final : public Compression {
public:
    ZlibCompress();
    ~ZlibCompress() override = default;

    size_t Compress(const void* input_data, size_t input_size, void* output_data, size_t output_size) override;

    size_t CompressBound(size_t input_size) override;  // 计算压缩后的大小

    std::string DeCompress(const void* input_data, size_t input_size) override;  // 解压缩

    void ResetStream() override;  // 重置压缩缓存区
private:
    void ResetDecompressStream();

private:
    std::unique_ptr<z_stream, ZStreamDeflateDeleter> compress_stream_;
    std::unique_ptr<z_stream, ZStreamInflateDeleter> decompress_stream_;
};
}  // namespace compress
}  // namespace logger