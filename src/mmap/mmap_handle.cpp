#include "mmap_handle.h"

#include <algorithm>
#include <string.h>

#include "utils/sys_util.h"

namespace logger {

constexpr size_t kDefaultCapacity = 512 * 1024;  // 默认大小 512k

MMapHandle::MMapHandle(fpath file_path) : file_path_(std::move(file_path)), handle_(nullptr), capacity_(0) {
    size_t file_size = fs::GetFileSize(file_path);
    size_t set_capacity_size = std::max(file_size, kDefaultCapacity);

    // 初始化一定会触发扩容 mmap映射
    Reserve_(set_capacity_size);

    Init_();
}

MMapHandle::MMapHeader* MMapHandle::Header_() const {
    if (!handle_) {
        return nullptr;
    }

    if (capacity_ < sizeof(MMapHeader)) {
        return nullptr;
    }

    return static_cast<MMapHeader*>(handle_);
}

uint8_t* MMapHandle::Data() const {
    if (!IsValid_()) {
        return nullptr;
    }

    return static_cast<uint8_t*>(handle_) + sizeof(MMapHeader);
}

void MMapHandle::Init_() {
    MMapHeader* header = Header_();
    if (!header) {
        return;
    }
    if (header->magic != MMapHeader::kMagic) {
        header->magic = MMapHeader::kMagic;
        header->size = 0;
    }
}

bool MMapHandle::Resize(size_t new_size) {
    if (!IsValid_()) {
        return false;
    }
    size_t need_capacity = sizeof(MMapHeader) + new_size;

    if (need_capacity < capacity_) {  // 容量未满不做处理
        return true;
    }

    // 容量满了 调整capacity
    if (Reserve_(need_capacity)) {  // 调整成功
        Header_()->size = new_size;
        return true;
    }

    return false;
}

size_t MMapHandle::Size() const {
    MMapHeader* header = Header_();
    if (header) {
        return header->size;
    }
    return 0;
}

bool MMapHandle::Push(const void* data, size_t data_size) {
    if (!IsValid_()) {
        return false;
    }

    size_t need_capacity = sizeof(MMapHeader) + Header_()->size + data_size;

    if (Reserve_(need_capacity)) {
        memcpy(Data() + Size(), data, data_size);

        Header_()->size += data_size;
        return true;
    }

    return false;
}

void MMapHandle::Clear() {
    if (!IsValid_()) {
        return;
    }
    Header_()->size = 0;
}

size_t MMapHandle::GetValidCapacity_(size_t size) {  // capacity 向上取虚拟内存页面倍数

    size_t page_size = GetPageSize();
    // 向上取虚拟内存页面倍数
    return ((size + page_size - 1) / page_size) * page_size;
}

bool MMapHandle::Reserve_(size_t target_capacity) {
    // target_capacity 向上取虚拟内存页面倍数
    target_capacity = GetValidCapacity_(target_capacity);

    if (target_capacity < capacity_) {
        return true;
    }

    size_t old_capacity = capacity_;
    target_capacity = old_capacity + std::max(old_capacity, target_capacity);  // 扩容策略 follow vector 策略

    // 解除映射
    Unmap_();

    // 重新mmap新大小
    if (TryMap_(target_capacity)) {
        capacity_ = target_capacity;
        return true;
    }

    return false;
}

bool MMapHandle::IsValid_() const {
    MMapHeader* header = Header_();
    if (!header) {
        return false;
    }
    return header->magic == MMapHeader::kMagic;
}
}  // namespace logger