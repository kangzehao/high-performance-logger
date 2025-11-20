#pragma once
#include "utils/file_util.h"

namespace logger {

class MMapHandle {
public:
    MMapHandle(fpath file_path);

    MMapHandle(const MMapHandle& other) = delete;
    MMapHandle& operator=(const MMapHandle& other) = delete;

    MMapHandle(MMapHandle&& other) = default;
    MMapHandle& operator=(MMapHandle&& other) = default;

    ~MMapHandle() = default;

    uint8_t* Data() const;

    bool Resize(size_t new_size);

    size_t Size() const;

    bool Push(const void* data, size_t data_size);

    void Clear();

    bool Empty() const {
        return Size() == 0;
    }

private:
    struct MMapHeader {
        static constexpr uint32_t kMagic = 0xdeadbeef;
        uint32_t magic = kMagic;
        size_t size;  // 存储数据大小
    };

    fpath file_path_;

    void* handle_;

    size_t capacity_;

private:
    MMapHeader* Header_() const;

    void Init_();

    size_t GetValidCapacity_(size_t size);

    size_t Capacity() const {
        return capacity_;
    }

    bool Reserve_(size_t target_capacity);

    bool TryMap_(size_t capacity);

    void Unmap_();

    void Sync_();  // 落盘

    bool IsValid_() const;
};
}  // namespace logger