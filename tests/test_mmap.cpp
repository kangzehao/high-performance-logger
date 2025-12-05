#include <gtest/gtest.h>
#include <vector>
#include <cstring>
#include <filesystem>
#include <fstream>

#include "internal_log.h"
#include "mmap/mmap_handle.h"

using namespace logger;

class MMapHandleTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 在每个测试前创建唯一的测试文件路径
        test_id_ = std::to_string(++test_counter_);
        test_file_ = std::filesystem::temp_directory_path() / ("test_mmap_handle_" + test_id_ + ".dat");

        // 确保文件不存在
        if (std::filesystem::exists(test_file_)) {
            std::filesystem::remove(test_file_);
        }

        // LOG_INFO("Test file: {}", test_file_.string());
    }

    void TearDown() override {
        // 测试结束后清理文件
        if (std::filesystem::exists(test_file_)) {
            std::filesystem::remove(test_file_);
        }
    }

    // 检查文件是否存在且大小 >= 指定值
    void AssertFileConsistent(size_t payload_size_at_least = 0) {
        EXPECT_TRUE(std::filesystem::exists(test_file_));
        auto sz = std::filesystem::file_size(test_file_);
        EXPECT_GE(sz, payload_size_at_least);
    }

    std::filesystem::path test_file_;
    std::string test_id_;
    static int test_counter_;
};

int MMapHandleTest::test_counter_ = 0;

TEST_F(MMapHandleTest, ConstructorAndEmpty) {
    MMapHandle mmap(test_file_);

    EXPECT_TRUE(mmap.Empty());
    EXPECT_EQ(mmap.Size(), 0);
    EXPECT_NE(mmap.Data(), nullptr);
    EXPECT_GE(mmap.Capacity(), mmap.Size());

    AssertFileConsistent(0);
}

TEST_F(MMapHandleTest, PushAndSize) {
    MMapHandle mmap(test_file_);
    std::vector<uint8_t> data = {1, 2, 3, 4, 5};

    size_t original_size = mmap.Size();
    bool ok = mmap.Push(data.data(), data.size());

    EXPECT_TRUE(ok);
    EXPECT_EQ(mmap.Size(), original_size + data.size());

    uint8_t* ptr = mmap.Data();
    ASSERT_NE(ptr, nullptr);

    // 验证末尾追加的数据与写入一致
    EXPECT_EQ(std::memcmp(ptr + original_size, data.data(), data.size()), 0);

    AssertFileConsistent(mmap.Size());
}

TEST_F(MMapHandleTest, PushAndReadString) {
    MMapHandle mmap(test_file_);
    std::string msg1 = "hello ";
    std::string msg2 = "mmap!";
    std::string combined = msg1 + msg2;

    // 原始内容
    size_t original_size = mmap.Size();
    std::string original(reinterpret_cast<char*>(mmap.Data()), mmap.Size());

    bool ok1 = mmap.Push(msg1.data(), msg1.size());
    bool ok2 = mmap.Push(msg2.data(), msg2.size());

    EXPECT_TRUE(ok1);
    EXPECT_TRUE(ok2);
    EXPECT_EQ(mmap.Size(), original_size + combined.size());

    std::string read(reinterpret_cast<char*>(mmap.Data()), mmap.Size());
    EXPECT_EQ(read, original + combined);

    AssertFileConsistent(mmap.Size());
}

TEST_F(MMapHandleTest, PushEmptyData) {
    MMapHandle mmap(test_file_);
    size_t original_size = mmap.Size();
    std::vector<uint8_t> empty;

    bool ok = mmap.Push(empty.data(), empty.size());

    EXPECT_TRUE(ok);
    EXPECT_EQ(mmap.Size(), original_size);
}

TEST_F(MMapHandleTest, ResizeExpandAndShrink) {
    MMapHandle mmap(test_file_);
    std::vector<uint8_t> data = {10, 20, 30};

    EXPECT_TRUE(mmap.Push(data.data(), data.size()));
    EXPECT_EQ(mmap.Size(), data.size());

    // 扩大
    size_t resize = mmap.Capacity() + 10;
    bool ok_expand = mmap.Resize(resize);

    EXPECT_TRUE(ok_expand);
    EXPECT_EQ(mmap.Size(), resize);
    EXPECT_GE(mmap.Capacity(), resize);

    // 收缩
    bool ok_shrink = mmap.Resize(2);

    EXPECT_TRUE(ok_shrink);
    EXPECT_EQ(mmap.Size(), 2);

    // 前两个字节应当仍是原来数据的前两个
    uint8_t* ptr = mmap.Data();
    EXPECT_EQ(ptr[0], data[0]);
    EXPECT_EQ(ptr[1], data[1]);

    AssertFileConsistent(mmap.Size());
}

TEST_F(MMapHandleTest, Clear) {
    MMapHandle mmap(test_file_);
    std::vector<uint8_t> data = {1, 2, 3};

    EXPECT_TRUE(mmap.Push(data.data(), data.size()));
    EXPECT_EQ(mmap.Size(), 3);

    mmap.Clear();

    EXPECT_EQ(mmap.Size(), 0);
    EXPECT_TRUE(mmap.Empty());

    // 映射仍应有效
    EXPECT_NE(mmap.Data(), nullptr);
    EXPECT_GE(mmap.Capacity(), 0);

    AssertFileConsistent(0);
}

TEST_F(MMapHandleTest, MultiplePushes) {
    MMapHandle mmap(test_file_);
    std::string s1 = "ABC";
    std::string s2 = "DEF";
    std::string s3 = "GHI";

    bool ok1 = mmap.Push(s1.data(), s1.size());
    bool ok2 = mmap.Push(s2.data(), s2.size());
    bool ok3 = mmap.Push(s3.data(), s3.size());

    EXPECT_TRUE(ok1);
    EXPECT_TRUE(ok2);
    EXPECT_TRUE(ok3);

    std::string read(reinterpret_cast<char*>(mmap.Data()), mmap.Size());
    EXPECT_EQ(read, s1 + s2 + s3);
}

TEST_F(MMapHandleTest, LargeDataPush) {
    MMapHandle mmap(test_file_);

    // 测试大数据推送
    std::vector<uint8_t> large_data(1024 * 1024);  // 1MB
    for (size_t i = 0; i < large_data.size(); ++i) {
        large_data[i] = i % 256;
    }

    bool ok = mmap.Push(large_data.data(), large_data.size());

    EXPECT_TRUE(ok);
    EXPECT_EQ(mmap.Size(), large_data.size());

    // 验证数据正确性
    uint8_t* ptr = mmap.Data();
    for (size_t i = 0; i < large_data.size(); ++i) {
        EXPECT_EQ(ptr[i], large_data[i]) << "Data mismatch at index " << i;
    }

    AssertFileConsistent(mmap.Size());
}

TEST_F(MMapHandleTest, FilePersistence) {
    // 测试数据持久化到文件
    std::vector<uint8_t> data = {100, 200, 255, 0, 128};

    {
        // 第一个作用域写入数据
        MMapHandle mmap(test_file_);
        EXPECT_TRUE(mmap.Push(data.data(), data.size()));
        EXPECT_EQ(mmap.Size(), data.size());
    }

    // 文件应该存在且包含数据
    AssertFileConsistent(data.size());

    {
        // 重新打开文件验证数据持久化
        MMapHandle mmap(test_file_);
        EXPECT_EQ(mmap.Size(), data.size());

        uint8_t* ptr = mmap.Data();
        EXPECT_EQ(std::memcmp(ptr, data.data(), data.size()), 0);
    }
}

// 死亡测试：测试无效参数
TEST_F(MMapHandleTest, InvalidParameters) {
    MMapHandle mmap(test_file_);

    // 测试空指针但大小为0（应该允许）
    EXPECT_TRUE(mmap.Push(nullptr, 0));

    // 测试无效的文件路径
    std::filesystem::path invalid_path = "/invalid/path/mmap_test.dat";

    // 注意：这取决于你的实现是否立即抛出异常
    // 如果构造函数会抛出异常，可以使用 EXPECT_THROW
    // EXPECT_THROW(MMapHandle invalid_mmap(invalid_path), std::exception);
}