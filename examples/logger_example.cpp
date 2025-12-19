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
