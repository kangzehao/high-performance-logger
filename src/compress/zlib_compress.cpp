#include "zlib_compress.h"

namespace logger {
namespace compress {
static bool IsZlibCompressed(const void* input_data, size_t input_size) {
    if (!input_data) {
        return false;
    }

    constexpr size_t kZlibHeadSize = 2;  // 2字节头部
    if (input_size < kZlibHeadSize) {
        return false;
    }

    // 判断是否有zlib头 避免使用magic (大小端序有差异)
    const unsigned char* bytes = static_cast<const unsigned char*>(input_data);

    // 检查第一个字节的低4位是否为8（DEFLATE方法），并且整个头部能被31整除
    // https://www.rfc-editor.org/rfc/rfc1950
    // FLG (FLaGs)
    //          This flag byte is divided as follows:

    //             bits 0 to 4  FCHECK  (check bits for CMF and FLG)
    //             bit  5       FDICT   (preset dictionary)
    //             bits 6 to 7  FLEVEL  (compression level)

    //          The FCHECK value must be such that CMF and FLG, when viewed as
    //          a 16-bit unsigned integer stored in MSB order (CMF*256 + FLG),
    //          is a multiple of 31.

    uint16_t cmf = bytes[0];  // 压缩方法
    uint16_t flg = bytes[1];  // 压缩级别
    uint16_t value = (cmf << 8) | flg;
    if ((cmf & 0x0F) == 8 && (value % 31 == 0)) {
        return true;
    }

    return false;
}

ZlibCompress::ZlibCompress() {
    ResetStream();
    ResetDecompressStream();
}

size_t ZlibCompress::Compress(const void* input_data, size_t input_size, void* output_data, size_t output_size) {
    if (!input_data || !output_data) {
        return 0;
    }

    if (!compress_stream_) {
        return 0;
    }

    compress_stream_->next_in = (Bytef*) input_data;
    compress_stream_->avail_in = input_size;

    compress_stream_->next_out = (Bytef*) output_data;
    compress_stream_->avail_out = output_size;

    int ret = Z_OK;

    do {
        ret = deflate(compress_stream_.get(), Z_SYNC_FLUSH);
        if (ret != Z_OK && ret != Z_BUF_ERROR && ret != Z_STREAM_END) {
            return 0;
        }
    } while (ret == Z_BUF_ERROR);  // zlib暂时无法压缩，重试

    size_t out_len = output_size - compress_stream_->avail_out;
    return out_len;
}

size_t ZlibCompress::CompressBound(size_t input_size) {
    return input_size + 10;
}

std::string ZlibCompress::DeCompress(const void* input_data, size_t input_size) {
    if (!input_data) {
        return "";
    }
    if (IsZlibCompressed(input_data, input_size)) {
        ResetDecompressStream();
    }
    if (!decompress_stream_) {
        return "";
    }

    decompress_stream_->next_in = (Bytef*) input_data;
    decompress_stream_->avail_in = input_size;

    std::string output;
    while (decompress_stream_->avail_in > 0) {
        char buffer[4096] = {0};
        decompress_stream_->next_out = (Bytef*) buffer;
        decompress_stream_->avail_out = sizeof(buffer);
        int ret = inflate(decompress_stream_.get(), Z_SYNC_FLUSH);
        if (ret != Z_OK && ret != Z_STREAM_END) {
            return "";
        }
        output.append(buffer, sizeof(buffer) - decompress_stream_->avail_out);
    }
    return output;
}

void ZlibCompress::ResetStream() {
    compress_stream_ = std::unique_ptr<z_stream, ZStreamDeflateDeleter>(new z_stream());
    compress_stream_->zalloc = Z_NULL;
    compress_stream_->zfree = Z_NULL;
    compress_stream_->opaque = Z_NULL;

    int32_t ret = deflateInit2(
            compress_stream_.get(), Z_BEST_COMPRESSION, Z_DEFLATED, MAX_WBITS, MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY);
    if (ret != Z_OK) {
        compress_stream_.reset();
    }
}

void ZlibCompress::ResetDecompressStream() {
    decompress_stream_ = std::unique_ptr<z_stream, ZStreamInflateDeleter>(new z_stream());
    decompress_stream_->zalloc = Z_NULL;
    decompress_stream_->zfree = Z_NULL;
    decompress_stream_->opaque = Z_NULL;
    decompress_stream_->avail_in = 0;
    decompress_stream_->next_in = Z_NULL;
    int32_t ret = inflateInit2(decompress_stream_.get(), MAX_WBITS);
    if (ret != Z_OK) {
        decompress_stream_.reset();
    }
}
}  // namespace compress
}  // namespace logger