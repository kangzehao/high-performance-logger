#include <iostream>
#include <vector>
#include <cstring>
#include <cassert>
#include <filesystem>

#include "internal_log.h"
#include "mmap/mmap_handle.h"

using namespace logger;

void test_constructor_and_empty() {
    std::filesystem::path test_file = "./test_mmap_handle.dat";
    MMapHandle mmap(test_file);
    assert(mmap.Empty());
    assert(mmap.Size() == 0);
    LOG_INFO("test_constructor_and_empty passed\n");
}

void test_push_and_size() {
    std::filesystem::path test_file = "./test_mmap_handle_push.dat";
    MMapHandle mmap(test_file);
    std::vector<uint8_t> data = {1, 2, 3, 4, 5};
    bool ok = mmap.Push(data.data(), data.size());
    assert(ok);
    assert(mmap.Size() == data.size());
    uint8_t* ptr = mmap.Data();
    assert(ptr != nullptr);
    assert(std::memcmp(ptr, data.data(), data.size()) == 0);
    LOG_INFO("test_push_and_size passed\n");
}

void test_push_and_read_string() {
    std::filesystem::path test_file = "./test_mmap_handle_string.dat";
    MMapHandle mmap(test_file);
    std::string msg = "hello mmap!";
    bool ok = mmap.Push(msg.data(), msg.size());
    assert(ok);
    assert(mmap.Size() == msg.size());
    uint8_t* ptr = mmap.Data();
    assert(ptr != nullptr);
    std::string read_str(reinterpret_cast<char*>(ptr), mmap.Size());
    assert(read_str == msg);
    LOG_INFO("test_push_and_read_string passed\n");
}

void test_resize() {
    std::filesystem::path test_file = "./test_mmap_handle_resize.dat";
    MMapHandle mmap(test_file);
    std::vector<uint8_t> data = {1, 2, 3};
    mmap.Push(data.data(), data.size());
    bool ok = mmap.Resize(10);
    assert(ok);
    assert(mmap.Size() == 10);
    LOG_INFO("test_resize passed\n");
}

void test_clear() {
    std::filesystem::path test_file = "./test_mmap_handle_clear.dat";
    MMapHandle mmap(test_file);
    std::vector<uint8_t> data = {1, 2, 3};
    mmap.Push(data.data(), data.size());
    mmap.Clear();
    assert(mmap.Size() == 0);
    assert(mmap.Empty());
    LOG_INFO("test_clear passed\n");
}

int main() {
    test_constructor_and_empty();
    test_push_and_size();
    test_push_and_read_string();
    test_resize();
    test_clear();
    LOG_INFO("All MMapHandle tests passed!\n");
    return 0;
}