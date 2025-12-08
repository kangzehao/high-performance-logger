#include "aes_crypt.h"

#include <cryptopp/aes.h>
#include <cryptopp/base64.h>
#include <cryptopp/cryptlib.h>
#include <cryptopp/eccrypto.h>
#include <cryptopp/filters.h>
#include <cryptopp/hex.h>
#include <cryptopp/modes.h>
#include <cryptopp/oids.h>
#include <cryptopp/osrng.h>

namespace logger {
namespace crypt {

// 隐藏内部实现细节
namespace detail {

static std::string GenerateKey() {
    CryptoPP::AutoSeededRandomPool rnd;

    CryptoPP::byte key[CryptoPP::AES::DEFAULT_KEYLENGTH];

    rnd.GenerateBlock(key, sizeof(key));

    return BinaryKeyToHex(std::string(reinterpret_cast<const char*>(key), sizeof(key)));
}

static std::string GenerateIV() {
    CryptoPP::AutoSeededRandomPool rnd;

    CryptoPP::byte iv[CryptoPP::AES::BLOCKSIZE];

    rnd.GenerateBlock(iv, sizeof(iv));

    return BinaryKeyToHex(std::string(reinterpret_cast<const char*>(iv), sizeof(iv)));
}

void Encrypt(const void* input_data,
             size_t input_size,
             std::string& output_data,
             const std::string& key,
             const std::string& iv) {
    // 初始化AES算法 编码
    CryptoPP::AES::Encryption aes_encryption(reinterpret_cast<const CryptoPP::byte*>(key.data()), key.size());

    // 初始化CBC模式 编码
    CryptoPP::CBC_Mode_ExternalCipher::Encryption cbc_encryption(aes_encryption,
                                                                 reinterpret_cast<const CryptoPP::byte*>(iv.data()));
    // 创建流式处理器
    // AES 只能加密16字节块
    // 大于16的数据进行分块，小于16字节进行填充，采用流式加密，块与块之间采取CBC模式，后一个块的加密依赖于前一个块
    CryptoPP::StreamTransformationFilter stf_encryptor(cbc_encryption, new CryptoPP::StringSink(output_data));

    // 填充待加密数据
    stf_encryptor.Put(reinterpret_cast<const CryptoPP::byte*>(input_data), input_size);

    // 完成加密，处理所有数据
    stf_encryptor.MessageEnd();
}

std::string Decrypt(const void* input_data, size_t input_size, const std::string& key, const std::string& iv) {
    // 解码器 与编码器工作流程类似
    std::string decryptedtext;

    // 初始化AES算法 解码
    CryptoPP::AES::Decryption aes_decryption(reinterpret_cast<const CryptoPP::byte*>(key.data()), key.size());

    // 初始化CBC模式 解码
    CryptoPP::CBC_Mode_ExternalCipher::Decryption cbc_decryption(aes_decryption,
                                                                 reinterpret_cast<const CryptoPP::byte*>(iv.data()));

    // 创建流式处理器
    CryptoPP::StreamTransformationFilter stf_decryptor(cbc_decryption, new CryptoPP::StringSink(decryptedtext));

    // 填充解码数据
    stf_decryptor.Put(reinterpret_cast<const CryptoPP::byte*>(input_data), input_size);

    // 完成解码
    stf_decryptor.MessageEnd();

    return decryptedtext;
}
}  // namespace detail

AESCrypt::AESCrypt(std::string key) {
    key_ = std::move(key);
    // iv_一般是每加密一次随即生成，iv可以明文传输给服务端解密，这样安全性更高
    // 固定也行，安全性够用
    iv_ = "dad0c0012340080a";
}

std::string AESCrypt::GenerateKey() {
    return detail::GenerateKey();
}
std::string AESCrypt::GenerateIV() {
    return detail::GenerateIV();
}

void AESCrypt::Encrypt(const void* input_data, size_t input_size, std::string& output_data) {
    detail::Encrypt(input_data, input_size, output_data, key_, iv_);
}
std::string AESCrypt::Decrypt(const void* input_data, size_t input_size) {
    return detail::Decrypt(input_data, input_size, key_, iv_);
}

}  // namespace crypt
}  // namespace logger