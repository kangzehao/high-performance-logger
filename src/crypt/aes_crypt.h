#pragma once

#include "crypt.h"

#include <string>

namespace logger {
namespace crypt {
class AESCrypt final : public Crypt {
public:
    AESCrypt(std::string key);
    ~AESCrypt() override = default;

    static std::string GenerateKey();
    static std::string GenerateIV();

    void Encrypt(const void* input_data, size_t input_size, std::string& output_data) override;
    std::string Decrypt(const void* input_data, size_t input_size) override;

private:
    std::string key_;
    std::string iv_;
};
}  // namespace crypt
}  // namespace logger