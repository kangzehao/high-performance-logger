#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <cstring>

#include "compress/zlib_compress.h"
#include "compress/zstd_compress.h"

using namespace logger::compress;

class ZlibCompressTest : public ::testing::Test {
protected:
    void SetUp() override {}
    ZlibCompress zc_;
};

class ZstdCompressTest : public ::testing::Test {
protected:
    void SetUp() override {}
    ZstdCompress zc_;
};

TEST_F(ZlibCompressTest, CompressAndDecompress_Roundtrip_Text) {
    std::string input = "The quick brown fox jumps over the lazy dog.";

    size_t bound = zc_.CompressBound(input.size());
    std::vector<uint8_t> out(bound, 0);

    size_t out_len = zc_.Compress(input.data(), input.size(), out.data(), out.size());
    ASSERT_GT(out_len, 0u);

    std::string decompressed = zc_.DeCompress(out.data(), out_len);
    EXPECT_EQ(decompressed, input);
}

TEST_F(ZlibCompressTest, CompressAndDecompress_Roundtrip_Binary) {
    std::vector<uint8_t> input = {0x00, 0x01, 0x02, 0xFF, 0xFE, 0x7F, 0x00, 0x10, 0x20, 0x30};

    size_t bound = zc_.CompressBound(input.size());
    std::vector<uint8_t> out(bound, 0);

    size_t out_len = zc_.Compress(input.data(), input.size(), out.data(), out.size());
    ASSERT_GT(out_len, 0u);

    std::string decompressed = zc_.DeCompress(out.data(), out_len);
    ASSERT_EQ(decompressed.size(), input.size());
    EXPECT_EQ(std::memcmp(decompressed.data(), input.data(), input.size()), 0);
}

TEST_F(ZlibCompressTest, Decompress_OnAlreadyCompressedHeaderDetection) {
    // 压缩一段数据后再次解压，验证 IsCompressed 分支的流重置逻辑
    std::string input = "zlib header detection check";
    size_t bound = zc_.CompressBound(input.size());
    std::vector<uint8_t> out(bound, 0);
    size_t out_len = zc_.Compress(input.data(), input.size(), out.data(), out.size());
    ASSERT_GT(out_len, 0u);

    // 直接用输出作为输入进行解压
    std::string decompressed = zc_.DeCompress(out.data(), out_len);
    EXPECT_EQ(decompressed, input);
}

TEST_F(ZlibCompressTest, Compress_NullPointers_ReturnZero) {
    std::string input = "abc";
    size_t bound = zc_.CompressBound(input.size());
    std::vector<uint8_t> out(bound, 0);

    // null input
    size_t out_len1 = zc_.Compress(nullptr, input.size(), out.data(), out.size());
    EXPECT_EQ(out_len1, 0u);

    // null output
    size_t out_len2 = zc_.Compress(input.data(), input.size(), nullptr, out.size());
    EXPECT_EQ(out_len2, 0u);
}

TEST_F(ZlibCompressTest, Decompress_NullInput_ReturnEmpty) {
    std::string res = zc_.DeCompress(nullptr, 10);
    EXPECT_TRUE(res.empty());
}

TEST_F(ZlibCompressTest, Decompress_WrongData_ReturnEmpty) {
    // 非压缩数据，且长度足够，IsCompressed 返回 false 导致不重置，若先前未初始化也应返回空
    std::vector<uint8_t> fake = {0x01, 0x02, 0x03, 0x04};
    std::string res = zc_.DeCompress(fake.data(), fake.size());
    EXPECT_TRUE(res.empty());
}

TEST_F(ZlibCompressTest, Compress_OutputTooSmall_HandleGracefully) {
    std::string input(1024, 'A');
    std::vector<uint8_t> small_out(8, 0);  // 很小的输出缓冲
    size_t out_len = zc_.Compress(input.data(), input.size(), small_out.data(), small_out.size());
    // 小缓冲可能产生 Z_BUF_ERROR 循环直到无法继续，结果可能为 0 或小于缓冲大小
    EXPECT_LE(out_len, small_out.size());
}

// 可选：多次压缩/解压验证流复用
TEST_F(ZlibCompressTest, MultipleCompressDecompress_Calls) {
    std::string s1 = "first";
    std::string s2 = std::string(200, 'x');

    // compress s1
    size_t b1 = zc_.CompressBound(s1.size());
    std::vector<uint8_t> o1(b1, 0);
    size_t l1 = zc_.Compress(s1.data(), s1.size(), o1.data(), o1.size());
    ASSERT_GT(l1, 0u);
    EXPECT_EQ(zc_.DeCompress(o1.data(), l1), s1);

    // compress s2
    size_t b2 = zc_.CompressBound(s2.size());
    std::vector<uint8_t> o2(b2, 0);
    size_t l2 = zc_.Compress(s2.data(), s2.size(), o2.data(), o2.size());
    ASSERT_GT(l2, 0u);
    EXPECT_EQ(zc_.DeCompress(o2.data(), l2), s2);
}

// zstd

TEST_F(ZstdCompressTest, CompressAndDecompress_Roundtrip_Text) {
    std::string input = "The quick brown fox jumps over the lazy dog.";

    size_t bound = zc_.CompressBound(input.size());
    std::vector<uint8_t> out(bound, 0);

    size_t out_len = zc_.Compress(input.data(), input.size(), out.data(), out.size());
    ASSERT_GT(out_len, 0u);

    std::string decompressed = zc_.DeCompress(out.data(), out_len);
    EXPECT_EQ(decompressed, input);
}

TEST_F(ZstdCompressTest, CompressAndDecompress_Roundtrip_Binary) {
    std::vector<uint8_t> input = {0x00, 0x01, 0x02, 0xFF, 0xFE, 0x7F, 0x00, 0x10, 0x20, 0x30};

    size_t bound = zc_.CompressBound(input.size());
    std::vector<uint8_t> out(bound, 0);

    size_t out_len = zc_.Compress(input.data(), input.size(), out.data(), out.size());
    ASSERT_GT(out_len, 0u);

    std::string decompressed = zc_.DeCompress(out.data(), out_len);
    ASSERT_EQ(decompressed.size(), input.size());
    EXPECT_EQ(std::memcmp(decompressed.data(), input.data(), input.size()), 0);
}

TEST_F(ZstdCompressTest, Decompress_OnAlreadyCompressedHeaderDetection) {
    // 压缩一段数据后再次解压，验证 IsCompressed 分支的流重置逻辑
    std::string input = "zlib header detection check";
    size_t bound = zc_.CompressBound(input.size());
    std::vector<uint8_t> out(bound, 0);
    size_t out_len = zc_.Compress(input.data(), input.size(), out.data(), out.size());
    ASSERT_GT(out_len, 0u);

    // 直接用输出作为输入进行解压
    std::string decompressed = zc_.DeCompress(out.data(), out_len);
    EXPECT_EQ(decompressed, input);
}

TEST_F(ZstdCompressTest, Compress_NullPointers_ReturnZero) {
    std::string input = "abc";
    size_t bound = zc_.CompressBound(input.size());
    std::vector<uint8_t> out(bound, 0);

    // null input
    size_t out_len1 = zc_.Compress(nullptr, input.size(), out.data(), out.size());
    EXPECT_EQ(out_len1, 0u);

    // null output
    size_t out_len2 = zc_.Compress(input.data(), input.size(), nullptr, out.size());
    EXPECT_EQ(out_len2, 0u);
}

TEST_F(ZstdCompressTest, Decompress_NullInput_ReturnEmpty) {
    std::string res = zc_.DeCompress(nullptr, 10);
    EXPECT_TRUE(res.empty());
}

TEST_F(ZstdCompressTest, Decompress_WrongData_ReturnEmpty) {
    // 非压缩数据，且长度足够，IsCompressed 返回 false 导致不重置，若先前未初始化也应返回空
    std::vector<uint8_t> fake = {0x01, 0x02, 0x03, 0x04};
    std::string res = zc_.DeCompress(fake.data(), fake.size());
    EXPECT_TRUE(res.empty());
}

TEST_F(ZstdCompressTest, Compress_OutputTooSmall_HandleGracefully) {
    std::string input(1024, 'A');
    std::vector<uint8_t> small_out(8, 0);  // 很小的输出缓冲
    size_t out_len = zc_.Compress(input.data(), input.size(), small_out.data(), small_out.size());
    // 小缓冲可能产生 Z_BUF_ERROR 循环直到无法继续，结果可能为 0 或小于缓冲大小
    EXPECT_LE(out_len, small_out.size());
}

// 可选：多次压缩/解压验证流复用
TEST_F(ZstdCompressTest, MultipleCompressDecompress_Calls) {
    std::string s1 = "first";
    std::string s2 = std::string(200, 'x');

    // compress s1
    size_t b1 = zc_.CompressBound(s1.size());
    std::vector<uint8_t> o1(b1, 0);
    size_t l1 = zc_.Compress(s1.data(), s1.size(), o1.data(), o1.size());
    ASSERT_GT(l1, 0u);
    EXPECT_EQ(zc_.DeCompress(o1.data(), l1), s1);

    // compress s2
    size_t b2 = zc_.CompressBound(s2.size());
    std::vector<uint8_t> o2(b2, 0);
    size_t l2 = zc_.Compress(s2.data(), s2.size(), o2.data(), o2.size());
    ASSERT_GT(l2, 0u);
    EXPECT_EQ(zc_.DeCompress(o2.data(), l2), s2);
}
