#pragma once

#include <string>
#include <tuple>

namespace logger {
namespace crypt {

// 生成私钥 公钥
std::tuple<std::string, std::string> GenerateECDHkeyPair();

// 计算共享密钥 ECDH椭圆曲线离散对数
std::string ComputeECDHSharedSecret(const std::string& private_key, const std::string& peer_public_key);

std::string BinaryKeyToHex(const std::string& binary_key);

std::string HexKeyToBinary(const std::string& hex_key);

class Crypt {
public:
    Crypt() = default;
    virtual ~Crypt() = default;

    virtual void Encrypt(const void* input_data, size_t input_size, std::string& output_data) = 0;
    virtual std::string Decrypt(const void* input_data, size_t input_size) = 0;
};

}  // namespace crypt
}  // namespace logger