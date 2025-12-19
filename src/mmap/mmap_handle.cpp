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
    Reserve(set_capacity_size);

    Init();
}

MMapHandle::MMapHeader* MMapHandle::Header() const {
    if (!handle_) {
        return nullptr;
    }

    if (capacity_ < sizeof(MMapHeader)) {
        return nullptr;
    }

    return static_cast<MMapHeader*>(handle_);
}

uint8_t* MMapHandle::Data() const {
    if (!IsValid()) {
        return nullptr;
    }

    return static_cast<uint8_t*>(handle_) + sizeof(MMapHeader);
}

void MMapHandle::Init() {
    MMapHeader* header = Header();
    if (!header) {
        return;
    }
    if (header->magic != MMapHeader::kMagic) {
        header->magic = MMapHeader::kMagic;
        header->size = 0;
    }
}

bool MMapHandle::Resize(size_t new_size) {
    if (!IsValid()) {
        return false;
    }
    size_t need_capacity = sizeof(MMapHeader) + new_size;

    if (need_capacity < capacity_) {  // 容量未满无需扩容, 只调整size大小
        Header()->size = new_size;
        return true;
    }

    // 容量满了 调整capacity
    if (Reserve(need_capacity)) {  // 超出容量，扩容
        Header()->size = new_size;
        return true;
    }

    return false;
}

size_t MMapHandle::Size() const {
    MMapHeader* header = Header();
    if (header) {
        return header->size;
    }
    return 0;
}

bool MMapHandle::Push(const void* data, size_t data_size) {
    if (!IsValid()) {
        return false;
    }

    size_t need_capacity = sizeof(MMapHeader) + Header()->size + data_size;

    if (Reserve(need_capacity)) {
        memcpy(Data() + Size(), data, data_size);

        Header()->size += data_size;
        return true;
    }

    return false;
}

void MMapHandle::Clear() {
    if (!IsValid()) {
        return;
    }
    Header()->size = 0;
}

size_t MMapHandle::GetValidCapacity(size_t size) {  // capacity 向上取虚拟内存页面倍数

    size_t page_size = GetPageSize();
    // 向上取虚拟内存页面倍数
    return ((size + page_size - 1) / page_size) * page_size;
}

bool MMapHandle::Reserve(size_t target_capacity) {
    // target_capacity 向上取虚拟内存页面倍数
    target_capacity = GetValidCapacity(target_capacity);

    if (target_capacity < capacity_) {
        return true;
    }

    size_t old_capacity = capacity_;
    target_capacity = old_capacity + std::max(old_capacity, target_capacity);  // 扩容策略 follow vector 策略

    // 解除映射
    Unmap();

    // 重新mmap新大小
    if (TryMap(target_capacity)) {
        capacity_ = target_capacity;
        return true;
    }

    return false;
}

bool MMapHandle::IsValid() const {
    MMapHeader* header = Header();
    if (!header) {
        return false;
    }
    return header->magic == MMapHeader::kMagic;
}

double MMapHandle::GetRatio() const {
    if (!IsValid()) {
        return 0.0;
    }

    double payload = static_cast<double>(Size());
    double cap = static_cast<double>(Capacity() - sizeof(MMapHeader));
    return cap > 0.0 ? payload / cap : 0.0;
}
}  // namespace logger