#include <gtest/gtest.h>
#include <memory>

#include "log_factory.h"
#include "sinks/console_sink.h"

using namespace logger;

TEST(LogFactoryTest, SingletonInstanceIsSame) {
    auto& a = LogFactory::GetInstacne();
    auto& b = LogFactory::GetInstacne();
    EXPECT_EQ(&a, &b);
}

TEST(LogFactoryTest, DefaultHandleIsNull) {
    auto& f = LogFactory::GetInstacne();
    EXPECT_EQ(f.GetLogHandle(), nullptr);
}

TEST(LogFactoryTest, SetAndGetHandle) {
    auto& f = LogFactory::GetInstacne();

    // 构造一个 ExtensionLogHandle，使用 ConsoleSink
    auto sink = std::make_shared<ConsoleSink>();
    auto handle = std::make_shared<ExtensionLogHandle>(sink);

    f.SetLogHandle(handle);

    ExtensionLogHandle* got = f.GetLogHandle();
    ASSERT_NE(got, nullptr);
    EXPECT_EQ(got, handle.get());
}
