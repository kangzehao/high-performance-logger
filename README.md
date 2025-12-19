# 🚀 Effective Logger

![Standard](https://img.shields.io/badge/standard-C%2B%2B17-blue.svg?style=flat-square&logo=c%2B%2B)
![License](https://img.shields.io/badge/license-MIT-green.svg?style=flat-square)
![Build](https://img.shields.io/badge/build-passing-brightgreen.svg?style=flat-square)
![Platform](https://img.shields.io/badge/platform-Linux-lightgrey.svg?style=flat-square)

**Effective Logger** 是一个基于 **C++17** 开发的高性能、异步、结构化日志库。专为**高并发**、**大数据量**写入场景设计。

与业界标杆（如 Spdlog）相比，Effective Logger 摒弃了传统的环形队列+互斥锁模式，采用 **Strand 串行化** 与 **双缓冲 (Double Buffering)** 架构。在多线程并发写入 4KB 大包的极端场景下，**吞吐量提升近 400%**，并彻底消除了长尾延迟。

同时，它原生支持 **Zstd 流式压缩** 与 **非对称加密**，实现了“极致性能”与“数据安全”的完美平衡。

---

## ✨ 核心特性 (Key Features)

* **⚡ 极致性能 (High Performance)**
    * 基于 **Strand 模型** 和 **双缓冲** 机制，将多线程竞争转化为流水线作业。
    * **无锁设计**：核心写入路径采用 `Thread Local` 预分配，杜绝锁竞争。
* **🛡️ 极低延迟 (Low Latency)**
    * 在 4 线程并发写入 4KB 数据场景下，延迟低至 **~5μs**。
    * **大包抗压**：数据包从 64B 增大至 4KB，写入耗时波动仅 **16%**（竞品劣化超 17 倍）。
* **🔒 安全增强 (Security & Compression)**
    * 内置 **Zstd** 压缩，显著减少磁盘占用。
    * 支持 **Crypto 非对称加密**（公钥配置），保障日志落盘即安全，防止敏感数据泄露。
* **💾 结构化与元数据**
    * 自动捕获 `SourceLocation`（文件名、行号、函数名）。
    * 支持结构化数据序列化（Protobuf ready）。
* **✅ 崩溃保护 (Crash Safe)**
    * 利用mmap的内核回写机制，确保进程在意外崩溃时日志数据的完整性。

---

## 📊 性能基准测试 (Benchmark)

我们在 **4 Core CPU @ 2.11GHz** 环境下，使用 Google Benchmark 对比了 **Effective Logger** 与 **Spdlog (Async Mode)**。

### 1. 核心场景：高并发大包写入 (4 Threads / 4KB)

这是生产环境中最考验日志库能力的场景。

| Metric | Spdlog (Async) | **Effective Logger** | 提升幅度 |
| :--- | :--- | :--- | :--- |
| **平均延迟 (Latency)** | 20.38 μs | **5.28 μs** | **↘ 降低 74%** |
| **吞吐量 (Throughput)** | ~4.9 万 OPS | **~18.9 万 OPS** | **🚀 提升 3.8 倍** |
| **CPU 利用率** | 低 (大量时间在等锁阻塞) | **极高 (满载处理业务)** | **零阻塞** |

### 2. 负载稳定性 (Scalability)

对比数据包大小从 **64 Bytes** 增长到 **4096 Bytes** 时，写入耗时的变化幅度。

| Payload | Spdlog 耗时变化 | **Effective 耗时变化** | 结论 |
| :--- | :--- | :--- | :--- |
| **64B $\to$ 4KB** | 0.82μs $\to$ 14.1μs (**涨 17 倍**) | 3.03μs $\to$ 3.52μs (**仅涨 16%**) | **大包场景完胜** |

> **📉 数据解读：**
> Spdlog 在处理大包时性能出现崩塌式下跌（RingBuffer 拷贝瓶颈），而 Effective Logger 表现出惊人的平稳性，几乎无视数据包大小的变化。

*(Benchmark raw data visualization)*
> *Benchmark 截图*

---

## 🛠️ 快速集成 (Quick Start)

### 依赖环境
* **Compiler**: C++17 (GCC 7+, Clang 6+, MSVC 2019+)
* **CMake**: >= 3.10
* **Libraries**:
    * Crypto++ (Encryption)
    * Zstd (Compression)
    * Protobuf (Serialization)
    * fmt (Formatting)
    * Google Benchmark (Optional, for testing)

### 编译安装

本项目使用 **Conan** 进行依赖管理。

```bash
# 1. 克隆仓库
git clone https://github.com/kangzehao/effective-logger
cd effective-logger

# 2. 一键构建 (自动安装依赖并编译)
chmod +x autobuild.sh
./autobuild.sh

# 或者手动构建
# conan install . --build=missing -s build_type=Release -of build
# cd build
# cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE="conan_toolchain.cmake"
# make -j8
```

### 📝 使用示例

```cpp
#include <iostream>
#include <memory>
#include "logger.h"
#include "console_sink.h"
#include "effective_sink.h"
#include "log_extension_handle.h"

std::string GenerateRandomString(int length) {
    std::string str;
    str.reserve(length);
    for (int i = 0; i < length; ++i) {
        str.push_back('a' + rand() % 26);
    }
    return str;
}

int main() {
    std::cout << "Logger Example Start!" << std::endl;
    std::shared_ptr<logger::Sink> sink = std::make_shared<logger::ConsoleSink>();
    logger::EffectiveSink::Config conf;
    conf.dir = "logs";
    conf.prefix = "loggerdemo";
    conf.pub_key =
            "04827405069030E26A211C973C8710E6FBE79B5CAA364AC111FB171311902277537F8852EADD17EB339EB7CD0BA2490A58CDED2C70"
            "2DFC1E"
            "FC7EDB544B869F039C";
    // private key FAA5BBE9017C96BF641D19D0144661885E831B5DDF52539EF1AB4790C05E665E

    {
        std::shared_ptr<logger::Sink> effective_sink = std::make_shared<logger::EffectiveSink>(conf);
        logger::LogHandle handle({effective_sink});
        std::string str = GenerateRandomString(10);

        auto begin = std::chrono::system_clock::now();
        for (int i = 0; i < 5; ++i) {
            if (i % 5 == 0) {
                std::cout << "i " << i << std::endl;
            }
            handle.Log(logger::LogLevel::kInfo, logger::SourceLocation{__FILE__, __LINE__, __FUNCTION__}, str);
        }
        effective_sink->Flush();
        auto end = std::chrono::system_clock::now();
        std::chrono::milliseconds diff = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin);
        std::cout << "our logger diff: " << diff.count() << std::endl;
    }

    std::cout << "Logger Example End!" << std::endl;
    return 0;
}

```

---

## ⚙️ 架构原理 (Architecture)

### 1. 双缓冲机制 (Double Buffering)
为了彻底将 I/O 延迟与业务线程隔离，我们设计了 **Master/Slave** 缓冲模型：
* **前端 (Master)**: 业务线程将日志写入 Thread-Local 的内存块，只有极小的原子操作开销。
* **后端 (Slave)**: 后台线程负责持有 Slave Buffer。
* **交换 (Swap)**: 当 Master 满或超时，指针瞬间交换。后台线程随后对 Slave Buffer 进行落盘。

### 2. Strand 模型 (无锁串行化)
不同于传统的 `Mutex` 抢锁机制，Effective Logger 采用类似 **Actor/Strand** 的设计。多线程请求被逻辑串行化，避免了操作系统层面的线程上下文切换（Context Switch）和锁竞争（Lock Contention），从而在高并发下实现了吞吐量的线性增长。

---

## 🤝 贡献与反馈

欢迎提交 Issue 和 Pull Request！我们特别关注以下方向的改进：
* 更多序列化协议的支持 (e.g., FlatBuffers)
* 跨平台支持 (Windows 尚未适配)

## 📄 License

本项目采用 [MIT License](LICENSE) 开源。商业使用请遵循开源协议。