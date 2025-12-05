#include "zstd_compress.h"

#include <cstring>

namespace logger {
namespace compress {

static bool IsZstdCompressed(const void* input_data, size_t input_size) {
    if (!input_data) {
        return false;
    }

    constexpr size_t kZstdHeadSize = 4;
    if (input_size < kZstdHeadSize) {
        return false;
    }

    // 直接比较字节（不依赖主机字节序）
    const uint8_t* bytes = static_cast<const uint8_t*>(input_data);
    if (bytes[0] == 0x28 && bytes[1] == 0xB5 && bytes[2] == 0x2F && bytes[3] == 0xFD) {
        return true;
    }

    return false;
}

ZstdCompress::ZstdCompress() {
    cctx_ = ZSTD_createCCtx();

    ZSTD_CCtx_setParameter(cctx_, ZSTD_c_compressionLevel, 5);

    dctx_ = ZSTD_createDCtx();
}

ZstdCompress::~ZstdCompress() {
    if (cctx_) {
        ZSTD_freeCCtx(cctx_);
    }

    if (dctx_) {
        ZSTD_freeDCtx(dctx_);
    }
}

size_t ZstdCompress::Compress(const void* input_data, size_t input_size, void* output_data, size_t output_size) {
    if (!input_data || input_size == 0 || !output_data) {
        return 0;
    }

    if (!cctx_) {
        return 0;
    }

    ZSTD_inBuffer input = {input_data, input_size, 0};
    ZSTD_outBuffer output_buffer = {output_data, output_size, 0};

    size_t ret = ZSTD_compressStream2(cctx_, &output_buffer, &input, ZSTD_e_flush);

    if (ZSTD_isError(ret) != 0) {
        return 0u;
    }
    return output_buffer.pos;
}

size_t ZstdCompress::CompressBound(size_t input_size) {
    return ZSTD_compressBound(input_size);
}

std::string ZstdCompress::DeCompress(const void* input_data, size_t input_size) {
    if (!input_data || input_size == 0) {
        return "";
    }
    if (IsZstdCompressed(input_data, input_size)) {
        ResetDecompressStream();
    }
    std::string output;
    output.reserve(10 * 1024);
    ZSTD_inBuffer input = {input_data, input_size, 0};
    ZSTD_outBuffer output_buffer = {
            const_cast<void*>(reinterpret_cast<const void*>(output.data())), output.capacity(), 0};
    size_t ret = ZSTD_decompressStream(dctx_, &output_buffer, &input);
    if (ZSTD_isError(ret) != 0) {
        return "";
    }
    output = std::string(reinterpret_cast<char*>(output_buffer.dst), output_buffer.pos);
    return output;
}

void ZstdCompress::ResetStream() {
    if (cctx_) {
        ZSTD_CCtx_reset(cctx_, ZSTD_reset_session_only);
    }
}

void ZstdCompress::ResetDecompressStream() {
    if (dctx_) {
        ZSTD_DCtx_reset(dctx_, ZSTD_reset_session_only);
    }
}
}  // namespace compress
}  // namespace logger