#pragma once

#include <filesystem>
#include <string>
#include <chrono>
#include <memory>
#include <filesystem>
#include <mutex>

#include "sink.h"
#include "space.h"
#include "log_msg.h"
#include "formatter.h"
#include "mmap_handle.h"
#include "zstd_compress.h"
#include "crypt.h"
#include "context.h"

namespace logger {
namespace detail {
struct ChunkHeader {
    static constexpr uint64_t kMagic = 0xdeadbeefdada1100;
    uint64_t magic;
    uint64_t size;
    char pub_key[128];

    ChunkHeader() : magic(kMagic), size(0) {}
};

struct ItemHeader {
    static constexpr uint32_t kMagic = 0xbe5fba11;
    uint32_t magic;
    uint32_t size;

    ItemHeader() : magic(kMagic), size(0) {}
};
}  // namespace detail

class EffectiveSink final : public Sink {
public:
    struct Config {
        std::filesystem::path dir;         // 文件目录
        std::string prefix;                // 文件名前缀，文件名命名格式：{prefix}_{datetime}.log
        std::string pub_key;               // 非对称加密公钥
        std::chrono::minutes interval{5};  // 淘汰查询间隔
        megabytes single_size{4};          // 单个日志大小 4M ->分片
        megabytes total_size{100};         // 总日志大小不超过100M 超过淘汰
    };

    explicit EffectiveSink(const Config& conf);
    ~EffectiveSink() = default;

    // 日志方法
    void Log(const LogMsg& msg) override;

    void SetFormatter(std::unique_ptr<Formatter> formatter) override;

    void Flush() override;

private:
    // 写入到缓存
    void WriteToCache(const void* data, uint32_t size);

    // 判断主cache容量
    bool NeedSwapCache();

    // 交换主从cache
    void SwapCache();

    // 缓存手动落盘
    void CacheToFile();

    // 异步落盘
    void PrepareCacheToFile();

    // 淘汰旧日志 定时任务
    void RemoveOldFile();

    std::filesystem::path GetLogFilePath();

private:
    Config conf_;
    std::mutex mutex_;

    std::unique_ptr<MMapHandle> master_cache_;
    std::unique_ptr<MMapHandle> slave_cache_;
    std::atomic<bool> slave_is_free_{true};

    std::unique_ptr<compress::Compression> compress_;
    std::unique_ptr<crypt::Crypt> crypt_;

    std::unique_ptr<Formatter> formatter_ptr_;

    ctx::TaskRunnerTag task_runner_;

    std::filesystem::path log_file_path_;

    std::string client_pub_key_;
    std::string compress_buf_;
    std::string encrypted_buf_;
};

};  // namespace logger