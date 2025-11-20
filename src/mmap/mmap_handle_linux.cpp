#include "mmap_handle.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include "internal_log.h"
#include "defer.h"

namespace logger {

bool MMapHandle::TryMap_(size_t capacity) {
    // 获取映射文件句柄 没有映射文件创建一个 （不覆盖）
    int fd = open(file_path_.string().c_str(), O_RDWR | O_CREAT, S_IRWXU);
    DEFER {
        if (fd) {
            close(fd);
        }
    };
    if (fd == -1) {
        LOG_ERROR("open file {} error: {}", file_path_.string(), fd);
        return false;
    }

    if (ftruncate(fd, capacity) == -1) {  // 设置文件为capacity大小
        LOG_ERROR("file {} resize {} fail", file_path_.string(), capacity);
        return false;
    }

    handle_ = mmap(NULL, capacity, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);  // 进行内存映射
    return handle_ != MAP_FAILED;
}

void MMapHandle::Unmap_() {
    if (handle_) {
        munmap(handle_, capacity_);
    }
    handle_ = NULL;
}

void MMapHandle::Sync_() {
    if (handle_) {
        msync(handle_, capacity_, MS_SYNC);
    }
}  // 页面缓存落盘
}  // namespace logger