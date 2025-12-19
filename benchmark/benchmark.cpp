#include "spdlog/spdlog.h"
#include "spdlog/async.h"
#include "spdlog/sinks/basic_file_sink.h"

#include <benchmark/benchmark.h>
#include <memory>
#include <string>
#include <vector>
#include <random>
#include <iostream>
#include <filesystem>

#include "logger.h"
#include "sinks/effective_sink.h"
#include "log_extension_handle.h"

// 全局资源管理 (Global Resources)
// 我们把所有 Logger 对象做成全局的，避免在 Benchmark 循环里反复创建销毁
// 这更符合真实服务原本的样子（Logger 伴随进程生命周期）

static std::shared_ptr<spdlog::logger> g_spdlog_sync;
static std::shared_ptr<spdlog::logger> g_spdlog_async;

static std::shared_ptr<logger::EffectiveSink> g_my_sink;
static std::unique_ptr<logger::LogHandle> g_my_logger;

// 辅助函数：生成随机字符串
std::string GenerateRandomString(int length) {
    static const char charset[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    std::string str;
    str.resize(length);
    for (int i = 0; i < length; ++i) {
        str[i] = charset[rand() % (sizeof(charset) - 1)];
    }
    return str;
}

// 初始化函数
void GlobalSetup() {
    // A. 准备目录
    if (!std::filesystem::exists("logs")) {
        std::filesystem::create_directory("logs");
    }

    // B. 初始化 Spdlog Sync
    try {
        g_spdlog_sync = spdlog::basic_logger_mt("sync_logger", "logs/spdlog_bench_sync.log", true);
        g_spdlog_sync->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");
    } catch (const std::exception& e) {
        std::cerr << "Init Sync Spdlog Failed: " << e.what() << std::endl;
    }

    // C. 初始化 Spdlog Async
    try {
        spdlog::init_thread_pool(8192, 1);  // 全局线程池
        g_spdlog_async = spdlog::create_async<spdlog::sinks::basic_file_sink_mt>(
                "async_logger", "logs/spdlog_bench_async.log", true);
        g_spdlog_async->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");
    } catch (const std::exception& e) {
        std::cerr << "Init Async Spdlog Failed: " << e.what() << std::endl;
    }

    // D. 初始化 Effective
    try {
        logger::EffectiveSink::Config conf;
        conf.dir = "logs";
        conf.prefix = "bench_mylog";
        conf.pub_key =
                "04827405069030E26A211C973C8710E6FBE79B5CAA364AC111FB171311902277537F8852EADD17EB339EB7CD0BA2490A58CDED"
                "2C70"
                "2DFC1E"
                "FC7EDB544B869F039C";

        g_my_sink = std::make_shared<logger::EffectiveSink>(conf);
        std::vector<std::shared_ptr<logger::Sink>> sinks = {g_my_sink};
        g_my_logger = std::make_unique<logger::LogHandle>(sinks.begin(), sinks.end());
    } catch (const std::exception& e) {
        std::cerr << "Init MyLogger Failed: " << e.what() << std::endl;
    }
}

// Benchmark Case 实现 (纯粹测写性能)
static void BM_Spdlog_Sync(benchmark::State& state) {
    // 预生成 Log 消息，不计入耗时
    std::string msg = GenerateRandomString(state.range(0));

    for (auto _ : state) {
        g_spdlog_sync->info(msg);
    }
}

static void BM_Spdlog_Async(benchmark::State& state) {
    std::string msg = GenerateRandomString(state.range(0));

    for (auto _ : state) {
        g_spdlog_async->info(msg);
    }
}

static void BM_Effectivelog(benchmark::State& state) {
    std::string msg = GenerateRandomString(state.range(0));
    logger::SourceLocation loc{__FILE__, __LINE__, __FUNCTION__};

    for (auto _ : state) {
        g_my_logger->Log(logger::LogLevel::kInfo, loc, msg);
    }
}

// 注册与运行
#define BENCH_OPTS RangeMultiplier(4)->Range(64, 4096)->UseRealTime()->Unit(benchmark::kNanosecond)

BENCHMARK(BM_Spdlog_Sync)->Threads(1)->BENCH_OPTS;
BENCHMARK(BM_Spdlog_Sync)->Threads(4)->BENCH_OPTS;

BENCHMARK(BM_Spdlog_Async)->Threads(1)->BENCH_OPTS;
BENCHMARK(BM_Spdlog_Async)->Threads(4)->BENCH_OPTS;

BENCHMARK(BM_Effectivelog)->Threads(1)->BENCH_OPTS;
BENCHMARK(BM_Effectivelog)->Threads(4)->BENCH_OPTS;

int main(int argc, char** argv) {
    // 1. 初始化所有 Logger
    GlobalSetup();

    // 2. 运行 Benchmark
    ::benchmark::Initialize(&argc, argv);
    if (::benchmark::ReportUnrecognizedArguments(argc, argv)) return 1;
    ::benchmark::RunSpecifiedBenchmarks();

    // 3. 进程退出前的清理 (Flush 落盘)
    std::cout << "Bench finished. Flushing logs..." << std::endl;
    if (g_spdlog_sync) g_spdlog_sync->flush();
    if (g_spdlog_async) g_spdlog_async->flush();
    if (g_my_sink) g_my_sink->Flush();

    // 4. 销毁资源
    spdlog::drop_all();
    spdlog::shutdown();
    g_my_logger.reset();
    g_my_sink.reset();

    return 0;
}