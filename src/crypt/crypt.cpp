#include "crypt.h"

#include <stdexcept>

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

// 生成私钥 公钥
std::tuple<std::string, std::string> GenerateECDHkeyPair() {
    CryptoPP::AutoSeededRandomPool rnd;
    // 定义对称加密算法ECDH，ECP类型的椭圆曲线
    CryptoPP::ECDH<CryptoPP::ECP>::Domain dh(CryptoPP::ASN1::secp256r1());

    // CryptoPP库的密钥对安全存储容器，析构及清空
    // STL容器析构后数据可能会残留在物理内存，等待下一次使用被覆盖
    CryptoPP::SecByteBlock priv(dh.PrivateKeyLength());
    CryptoPP::SecByteBlock pub(dh.PublicKeyLength());

    // 生成密钥对
    // 私钥 = 随机整数  公钥 = 私钥 + 椭圆曲线推导 -> ECDH
    dh.GenerateKeyPair(rnd, priv, pub);

    return std::make_tuple(std::string(reinterpret_cast<const char*>(priv.data()), priv.size()),
                           std::string(reinterpret_cast<const char*>(pub.data()), pub.size()));
}

// 计算共享密钥 ECDH椭圆曲线离散对数
std::string ComputeECDHSharedSecret(const std::string& private_key, const std::string& peer_public_key) {
    // 定义对称加密算法ECDH，ECP类型的椭圆曲线
    CryptoPP::ECDH<CryptoPP::ECP>::Domain dh(CryptoPP::ASN1::secp256r1());

    CryptoPP::SecByteBlock priv(reinterpret_cast<const CryptoPP::byte*>(private_key.data()), private_key.size());
    CryptoPP::SecByteBlock pub(reinterpret_cast<const CryptoPP::byte*>(peer_public_key.data()), peer_public_key.size());
    CryptoPP::SecByteBlock share(dh.AgreedValueLength());
    // agree 共享密钥计算
    if (!dh.Agree(share, priv, pub)) {
        throw std::runtime_error("Failed to compute shared secret");
    }
    return std::string(reinterpret_cast<const char*>(share.data()), share.size());
}

std::string BinaryKeyToHex(const std::string& binary_key) {
    std::string hex_key;
    CryptoPP::HexEncoder encoder;
    // 设置编码器输出对象
    encoder.Attach(new CryptoPP::StringSink(hex_key));
    // 处理对象传入编码器
    encoder.Put(reinterpret_cast<const CryptoPP::byte*>(binary_key.data()), binary_key.size());
    // 确保数据被处理完毕
    encoder.MessageEnd();
    return hex_key;
}

std::string HexKeyToBinary(const std::string& hex_key) {
    std::string binary_key;
    CryptoPP::HexDecoder decoder;
    // 设置解码器输出对象
    decoder.Attach(new CryptoPP::StringSink(binary_key));
    // 处理对象传入解码器
    decoder.Put(reinterpret_cast<const CryptoPP::byte*>(hex_key.data()), hex_key.size());
    // 确保数据被处理完毕
    decoder.MessageEnd();
    return binary_key;
}

}  // namespace crypt
}  // namespace logger