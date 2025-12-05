#pragma once

#include <memory>
#include <zstd.h>

#include "compress.h"

namespace logger {
namespace compress {

class ZstdCompress final : public Compression {
public:
    ZstdCompress();
    ~ZstdCompress() override;

    size_t Compress(const void* input_data, size_t input_size, void* output_data, size_t output_size) override;

    size_t CompressBound(size_t input_size) override;  // 计算压缩后的大小

    std::string DeCompress(const void* input_data, size_t input_size) override;  // 解压缩

    void ResetStream() override;  // 重置压缩缓存区

private:
    void ResetDecompressStream();

private:
    ZSTD_CCtx* cctx_;
    ZSTD_DCtx* dctx_;
};
}  // namespace compress
}  // namespace logger