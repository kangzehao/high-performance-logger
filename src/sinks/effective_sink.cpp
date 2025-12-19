#include "effective_sink.h"

#include "sys_util.h"

#include <tuple>
#include <cstring>
#include <fstream>

#include "internal_log.h"
#include "effective_formatter.h"
#include "aes_crypt.h"
#include "timer_count.h"

namespace logger {
EffectiveSink::EffectiveSink(const Config& conf) : conf_(std::move(conf)) {
    LOG_INFO("EffectiveSink: dir={}, prefix={}, pub_key={}, interval={}, single_size={}, total_size={}",
             conf_.dir.string(),
             conf_.prefix,
             conf_.pub_key,
             conf_.interval.count(),
             conf_.single_size.count(),
             conf_.total_size.count());
    if (!std::filesystem::exists(conf_.dir)) {
        std::filesystem::create_directories(conf_.dir);
    }

    // auto ecdh_key = crypt::GenerateECDHkeyPair();
    const auto& [client_pri, client_pub] = crypt::GenerateECDHkeyPair();
    client_pub_key_ = client_pub;
    LOG_INFO("EffectiveSink: client pub size {}", client_pub_key_.size());
    std::string svr_pub_key_bin = crypt::HexKeyToBinary(conf_.pub_key);
    std::string shared_secret = crypt::ComputeECDHSharedSecret(client_pri, svr_pub_key_bin);

    crypt_ = std::make_unique<crypt::AESCrypt>(shared_secret);
    compress_ = std::make_unique<compress::ZstdCompress>();

    formatter_ptr_ = std::make_unique<EffectiveFormatter>();

    task_runner_ = CREATE_NEW_TASK_RUNNER;

    master_cache_ = std::make_unique<MMapHandle>(conf_.dir / "master_cache");
    slave_cache_ = std::make_unique<MMapHandle>(conf_.dir / "slave_cache");

    if (!master_cache_ || !slave_cache_) {
        throw std::runtime_error("EffectiveSink::EffectiveSink: create mmap failed");
    }

    if (!slave_cache_->Empty()) {
        slave_is_free_.store(false);
        PrepareCacheToFile();
        WAIT_TASK_IDLE(task_runner_);  // 有可能主从都是有数据的，后续还需要处理主cache
    }

    if (!master_cache_->Empty()) {
        if (slave_cache_->Empty()) {
            slave_is_free_.store(false);
            SwapCache();
        }
        PrepareCacheToFile();
        // WAIT_TASK_IDLE(task_runner_);
    }

    POST_REPEATED_TASK(task_runner_, [this]() { RemoveOldFile(); }, conf_.interval, -1);
}

// 日志方法
void EffectiveSink::Log(const LogMsg& msg) {
    static thread_local MemoryBuffer buf;

    formatter_ptr_->Format(msg, &buf);

    // 压缩 + 加密
    if (master_cache_->Empty()) {
        compress_->ResetStream();
    }
    {
        std::lock_guard<std::mutex> lock(mutex_);
        // 压缩器输出最坏情况所需空间大小
        compress_buf_.reserve(compress_->CompressBound(buf.size()));
        size_t real_compress_buf_size =
                compress_->Compress(buf.data(), buf.size(), compress_buf_.data(), compress_buf_.capacity());
        if (!real_compress_buf_size) {
            LOG_ERROR("EffectiveSink::Log: compress failed");
        }

        encrypted_buf_.clear();
        size_t kAuthenticationTag = 16;
        encrypted_buf_.reserve(real_compress_buf_size + kAuthenticationTag);
        crypt_->Encrypt(compress_buf_.data(), real_compress_buf_size, encrypted_buf_);
        if (encrypted_buf_.empty()) {
            LOG_ERROR("EffectiveSink::Log: encrypt failed");
            return;
        }
        WriteToCache(encrypted_buf_.data(), encrypted_buf_.size());
    }

    if (NeedSwapCache()) {
        if (slave_cache_->Empty()) {
            slave_is_free_.store(false);
            SwapCache();
        }
        PrepareCacheToFile();
    }
    // master 不为空 slave也不为空， master超过capacity会触发扩容
}

void EffectiveSink::SetFormatter(std::unique_ptr<Formatter> formatter) {}

void EffectiveSink::Flush() {
    TIMER_COUNT("Flush");
    PrepareCacheToFile();
    WAIT_TASK_IDLE(task_runner_);

    if (slave_is_free_.load()) {
        slave_is_free_.store(false);
        SwapCache();
    }
    PrepareCacheToFile();
    WAIT_TASK_IDLE(task_runner_);
}

// 写入到缓存
void EffectiveSink::WriteToCache(const void* data, uint32_t size) {
    // 流式存储 需要head界定边界
    detail::ItemHeader head;
    head.size = size;
    master_cache_->Push(&head, sizeof(head));
    master_cache_->Push(data, size);
}

// 判断主cache容量
bool EffectiveSink::NeedSwapCache() {
    return master_cache_->GetRatio() > 0.8;
}

// 交换主从cache
void EffectiveSink::SwapCache() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::swap(master_cache_, slave_cache_);
}

// slave缓存手动落盘
void EffectiveSink::CacheToFile() {
    TIMER_COUNT("CacheToFile");
    if (slave_is_free_.load()) {
        return;
    }

    if (slave_cache_->Empty()) {
        slave_is_free_.store(true);
        return;
    }

    // 落盘 这里不需要加锁了，只要slave不为空 不会发生主从cache的交换，异步调用CacheToFile 会先将slave_is_free_
    // 置为false 文件头需要存放公钥 以便解析段解析
    auto file_path = GetLogFilePath();
    detail::ChunkHeader chunk_head;
    chunk_head.size = slave_cache_->Size();
    memcpy(chunk_head.pub_key, client_pub_key_.data(), client_pub_key_.size());

    // 系统调用 主要开销在这里，所以需要放到调度器里面异步执行
    std::ofstream ofs(file_path, std::ios::binary | std::ios::app);
    ofs.write(reinterpret_cast<char*>(&chunk_head), sizeof(chunk_head));
    ofs.write(reinterpret_cast<char*>(slave_cache_->Data()), chunk_head.size);
    ofs.close();

    slave_cache_->Clear();
    slave_is_free_.store(true);
}

// 异步落盘
void EffectiveSink::PrepareCacheToFile() {
    POST_TASK(task_runner_, [this]() { CacheToFile(); });
}

// 淘汰旧日志 定时任务
void EffectiveSink::RemoveOldFile() {
    LOG_INFO("EffectiveSink::ElimateFiles_: start");
    std::vector<std::filesystem::path> files;
    for (auto& p : std::filesystem::directory_iterator(conf_.dir)) {
        if (p.path().extension() == ".log") {
            files.push_back(p.path());
        }
    }

    std::sort(files.begin(), files.end(), [](const std::filesystem::path& lhs, const std::filesystem::path& rhs) {
        return std::filesystem::last_write_time(lhs) > std::filesystem::last_write_time(rhs);
    });

    size_t total_bytes = space_cast<bytes>(conf_.total_size).count();
    size_t used_bytes = 0;
    for (auto& file : files) {
        used_bytes += fs::GetFileSize(file);
        if (used_bytes > total_bytes) {
            LOG_INFO("EffectiveSink::ElimateFiles_: remove file={}", file.string());
            std::filesystem::remove(file);
        }
    }
}

// 文件命名拼接
std::filesystem::path EffectiveSink::GetLogFilePath() {
    auto GetDateTimePath = [this]() -> std::filesystem::path {
        std::time_t now = std::time(nullptr);
        std::tm tm;
        LocalTime(&tm, &now);
        char time_buf[32] = {0};
        std::strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", &tm);
        return (conf_.dir / (conf_.prefix + "_" + time_buf));
    };

    if (log_file_path_.empty()) {
        log_file_path_ = GetDateTimePath().string() + ".log";
    } else {  // 分片策略，日志是否需要分片->创建新日志
        auto file_size = fs::GetFileSize(log_file_path_);
        bytes single_bytes = space_cast<bytes>(conf_.single_size);

        // 需要分片
        if (file_size > single_bytes.count()) {
            auto date_time_path = GetDateTimePath();
            auto file_path = date_time_path.string() + ".log";
            // 颗粒度到秒，一秒内可能写入大量文件，需要加上索引
            if (std::filesystem::exists(file_path)) {
                int index = 0;
                for (auto& p : std::filesystem::directory_iterator(conf_.dir)) {
                    if (p.path().filename().string().find(date_time_path.string()) != std::string::npos) {
                        index++;
                    }
                }
                log_file_path_ = date_time_path.string() + "_" + std::to_string(index) + ".log";
            } else {
                log_file_path_ = file_path;
            }
        }
    }
    LOG_INFO("EffectiveSink::GetFilePath: log_file_path={}", log_file_path_.string());
    return log_file_path_;
}
}  // namespace logger