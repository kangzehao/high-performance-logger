#include <gtest/gtest.h>
#include <cryptopp/cryptlib.h>

#include "crypt/aes_crypt.h"
#include "crypt/crypt.h"

using namespace logger::crypt;

// ============================================================================
// 测试 ECDH 密钥生成和交换
// ============================================================================

class ECDHTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 每个测试前的准备工作
    }

    void TearDown() override {
        // 每个测试后的清理工作
    }
};

// 测试: ECDH 密钥对生成
TEST_F(ECDHTest, GenerateECDHkeyPair_GeneratesValidKeyPair) {
    // Arrange & Act
    auto [private_key, public_key] = GenerateECDHkeyPair();

    // Assert
    EXPECT_FALSE(private_key.empty()) << "Private key should not be empty";
    EXPECT_FALSE(public_key.empty()) << "Public key should not be empty";
    EXPECT_EQ(private_key.size(), 32) << "Private key should be 32 bytes";
    EXPECT_EQ(public_key.size(), 65) << "Public key should be 65 bytes (uncompressed format)";
}

// 测试: 每次生成的密钥对都不同
TEST_F(ECDHTest, GenerateECDHkeyPair_GeneratesUniqueKeys) {
    // Arrange & Act
    auto [pri1, pub1] = GenerateECDHkeyPair();
    auto [pri2, pub2] = GenerateECDHkeyPair();

    // Assert
    EXPECT_NE(pri1, pri2) << "Private keys should be unique";
    EXPECT_NE(pub1, pub2) << "Public keys should be unique";
}

// 测试: ECDH 共享密钥协商 - 双方得到相同密钥
TEST_F(ECDHTest, ComputeECDHSharedSecret_ProducesSameKeyForBothParties) {
    // Arrange
    auto [client_pri, client_pub] = GenerateECDHkeyPair();
    auto [server_pri, server_pub] = GenerateECDHkeyPair();

    // Act
    std::string client_shared = ComputeECDHSharedSecret(client_pri, server_pub);
    std::string server_shared = ComputeECDHSharedSecret(server_pri, client_pub);

    // Assert
    EXPECT_EQ(client_shared, server_shared) << "Both parties should derive the same shared secret";
    EXPECT_EQ(client_shared.size(), 32) << "Shared secret should be 32 bytes";
}

// 测试: 不同的密钥对产生不同的共享密钥
TEST_F(ECDHTest, ComputeECDHSharedSecret_DifferentKeyPairsProduceDifferentSecrets) {
    // Arrange
    auto [client_pri, client_pub] = GenerateECDHkeyPair();
    auto [server1_pri, server1_pub] = GenerateECDHkeyPair();
    auto [server2_pri, server2_pub] = GenerateECDHkeyPair();

    // Act
    std::string shared1 = ComputeECDHSharedSecret(client_pri, server1_pub);
    std::string shared2 = ComputeECDHSharedSecret(client_pri, server2_pub);

    // Assert
    EXPECT_NE(shared1, shared2) << "Different key pairs should produce different shared secrets";
}

// 测试: 使用无效的公钥应该抛出异常
TEST_F(ECDHTest, ComputeECDHSharedSecret_InvalidPublicKeyThrowsException) {
    // Arrange
    auto [client_pri, client_pub] = GenerateECDHkeyPair();
    std::string invalid_pub = "invalid_public_key_data";

    // Act & Assert
    EXPECT_THROW(ComputeECDHSharedSecret(client_pri, invalid_pub), std::runtime_error)
            << "Should throw exception for invalid public key";
}

// ============================================================================
// 测试十六进制编解码
// ============================================================================

class HexEncodingTest : public ::testing::Test {};

// 测试: 二进制转十六进制
TEST_F(HexEncodingTest, BinaryKeyToHex_ConvertsCorrectly) {
    // Arrange
    std::string binary = "\x01\x23\x45\x67\x89\xAB\xCD\xEF";

    // Act
    std::string hex = BinaryKeyToHex(binary);

    // Assert
    EXPECT_EQ(hex, "0123456789ABCDEF") << "Hex encoding should be correct";
}

// 测试: 十六进制转二进制
TEST_F(HexEncodingTest, HexKeyToBinary_ConvertsCorrectly) {
    // Arrange
    std::string hex = "0123456789ABCDEF";

    // Act
    std::string binary = HexKeyToBinary(hex);

    // Assert
    EXPECT_EQ(binary, "\x01\x23\x45\x67\x89\xAB\xCD\xEF") << "Binary decoding should be correct";
}

// 测试: 编解码往返转换
TEST_F(HexEncodingTest, HexEncoding_RoundTripConversion) {
    // Arrange
    auto [private_key, public_key] = GenerateECDHkeyPair();

    // Act
    std::string hex = BinaryKeyToHex(public_key);
    std::string restored = HexKeyToBinary(hex);

    // Assert
    EXPECT_EQ(public_key, restored) << "Round-trip conversion should preserve data";
}

// 测试: 空字符串转换
TEST_F(HexEncodingTest, HexEncoding_EmptyString) {
    // Arrange
    std::string empty;

    // Act
    std::string hex = BinaryKeyToHex(empty);
    std::string binary = HexKeyToBinary("");

    // Assert
    EXPECT_TRUE(hex.empty()) << "Empty input should produce empty hex";
    EXPECT_TRUE(binary.empty()) << "Empty hex should produce empty binary";
}

// ============================================================================
// 测试 AES 加密
// ============================================================================

class AESCryptTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 生成测试用的密钥
        test_key_ = std::string(32, 'K');  // 32字节密钥用于测试
    }

    std::string test_key_;
};

// 测试: AES 密钥生成
TEST_F(AESCryptTest, GenerateKey_ProducesValidKey) {
    // Act
    std::string key = AESCrypt::GenerateKey();

    // Assert
    EXPECT_FALSE(key.empty()) << "Generated key should not be empty";
    EXPECT_EQ(key.size(), 32) << "Hex key should be 32 characters (16 bytes * 2)";

    // 验证是否为有效的十六进制字符串
    for (char c : key) {
        EXPECT_TRUE((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F')) << "Key should only contain hex characters";
    }
}

// 测试: 每次生成的密钥都不同
TEST_F(AESCryptTest, GenerateKey_ProducesUniqueKeys) {
    // Act
    std::string key1 = AESCrypt::GenerateKey();
    std::string key2 = AESCrypt::GenerateKey();

    // Assert
    EXPECT_NE(key1, key2) << "Generated keys should be unique";
}

// 测试: IV 生成
TEST_F(AESCryptTest, GenerateIV_ProducesValidIV) {
    // Act
    std::string iv = AESCrypt::GenerateIV();

    // Assert
    EXPECT_FALSE(iv.empty()) << "Generated IV should not be empty";
    EXPECT_EQ(iv.size(), 32) << "Hex IV should be 32 characters (16 bytes * 2)";
}

// 测试: 每次生成的 IV 都不同
TEST_F(AESCryptTest, GenerateIV_ProducesUniqueIVs) {
    // Act
    std::string iv1 = AESCrypt::GenerateIV();
    std::string iv2 = AESCrypt::GenerateIV();

    // Assert
    EXPECT_NE(iv1, iv2) << "Generated IVs should be unique";
}

// 测试: 基本加密和解密
TEST_F(AESCryptTest, EncryptDecrypt_BasicFunctionality) {
    // Arrange
    AESCrypt cipher(test_key_);
    std::string plaintext = "Hello, World!";
    std::string ciphertext;

    // Act - 加密
    cipher.Encrypt(plaintext.data(), plaintext.size(), ciphertext);

    // Assert - 验证密文
    EXPECT_FALSE(ciphertext.empty()) << "Ciphertext should not be empty";
    EXPECT_NE(plaintext, ciphertext) << "Ciphertext should differ from plaintext";

    // Act - 解密
    std::string decrypted = cipher.Decrypt(ciphertext.data(), ciphertext.size());

    // Assert - 验证解密结果
    EXPECT_EQ(plaintext, decrypted) << "Decrypted text should match original plaintext";
}

// 测试: 加密空字符串
TEST_F(AESCryptTest, EncryptDecrypt_EmptyString) {
    // Arrange
    AESCrypt cipher(test_key_);
    std::string plaintext;
    std::string ciphertext;

    // Act
    cipher.Encrypt(plaintext.data(), plaintext.size(), ciphertext);
    std::string decrypted = cipher.Decrypt(ciphertext.data(), ciphertext.size());

    // Assert
    EXPECT_EQ(plaintext, decrypted) << "Empty string should decrypt correctly";
}

// 测试: 加密不同长度的数据
TEST_F(AESCryptTest, EncryptDecrypt_VariousLengths) {
    // Arrange
    AESCrypt cipher(test_key_);
    std::vector<size_t> lengths = {1, 15, 16, 17, 31, 32, 100, 1000};

    for (size_t len : lengths) {
        // Arrange
        std::string plaintext(len, 'A');
        std::string ciphertext;

        // Act
        cipher.Encrypt(plaintext.data(), plaintext.size(), ciphertext);
        std::string decrypted = cipher.Decrypt(ciphertext.data(), ciphertext.size());

        // Assert
        EXPECT_EQ(plaintext, decrypted) << "Failed for length: " << len;
    }
}

// 测试: 包含二进制数据的加密
TEST_F(AESCryptTest, EncryptDecrypt_BinaryData) {
    // Arrange
    AESCrypt cipher(test_key_);
    std::string plaintext;
    for (int i = 0; i < 256; i++) {
        plaintext += static_cast<char>(i);
    }
    std::string ciphertext;

    // Act
    cipher.Encrypt(plaintext.data(), plaintext.size(), ciphertext);
    std::string decrypted = cipher.Decrypt(ciphertext.data(), ciphertext.size());

    // Assert
    EXPECT_EQ(plaintext, decrypted) << "Binary data should be preserved";
}

// 测试: 包含空字符的数据
TEST_F(AESCryptTest, EncryptDecrypt_DataWithNullBytes) {
    // Arrange
    AESCrypt cipher(test_key_);
    std::string plaintext = "Hello\x00World\x00Test";
    plaintext.resize(16);  // 确保包含 null 字节
    std::string ciphertext;

    // Act
    cipher.Encrypt(plaintext.data(), plaintext.size(), ciphertext);
    std::string decrypted = cipher.Decrypt(ciphertext.data(), ciphertext.size());

    // Assert
    EXPECT_EQ(plaintext.size(), decrypted.size()) << "Length should be preserved";
    EXPECT_EQ(plaintext, decrypted) << "Data with null bytes should be preserved";
}

// 测试: 相同明文加密两次结果相同（因为 IV 固定）
TEST_F(AESCryptTest, Encrypt_SamePlaintextProducesSameCiphertext) {
    // Arrange
    AESCrypt cipher(test_key_);
    std::string plaintext = "Test message";
    std::string ciphertext1, ciphertext2;

    // Act
    cipher.Encrypt(plaintext.data(), plaintext.size(), ciphertext1);
    cipher.Encrypt(plaintext.data(), plaintext.size(), ciphertext2);

    // Assert
    EXPECT_EQ(ciphertext1, ciphertext2) << "Same plaintext should produce same ciphertext (with fixed IV)";
}

// 测试: 不同密钥产生不同密文
TEST_F(AESCryptTest, Encrypt_DifferentKeysProduceDifferentCiphertext) {
    // Arrange
    std::string key1(32, 'A');
    std::string key2(32, 'B');
    AESCrypt cipher1(key1);
    AESCrypt cipher2(key2);
    std::string plaintext = "Test message";
    std::string ciphertext1, ciphertext2;

    // Act
    cipher1.Encrypt(plaintext.data(), plaintext.size(), ciphertext1);
    cipher2.Encrypt(plaintext.data(), plaintext.size(), ciphertext2);

    // Assert
    EXPECT_NE(ciphertext1, ciphertext2) << "Different keys should produce different ciphertext";
}

// 测试: 使用错误的密钥无法正确解密
TEST_F(AESCryptTest, Decrypt_WrongKeyThrowsException) {
    // Arrange
    std::string key1(32, 'A');
    std::string key2(32, 'B');
    AESCrypt cipher1(key1);
    AESCrypt cipher2(key2);
    std::string plaintext = "Test message";
    std::string ciphertext;

    // Act
    cipher1.Encrypt(plaintext.data(), plaintext.size(), ciphertext);

    // Assert：期望解密时抛出异常
    EXPECT_THROW(cipher2.Decrypt(ciphertext.data(), ciphertext.size()),
                 CryptoPP::Exception  // 或者更具体的异常类型
                 )
            << "Wrong key should throw exception during decryption";
}

// ============================================================================
// 测试 ECDH + AES 集成
// ============================================================================

class ECDHAESIntegrationTest : public ::testing::Test {};

// 测试: 完整的密钥交换和加密流程
TEST_F(ECDHAESIntegrationTest, FullEncryptionFlow) {
    // Arrange - 密钥交换
    auto [client_pri, client_pub] = GenerateECDHkeyPair();
    auto [server_pri, server_pub] = GenerateECDHkeyPair();

    std::string client_shared = ComputeECDHSharedSecret(client_pri, server_pub);
    std::string server_shared = ComputeECDHSharedSecret(server_pri, client_pub);

    ASSERT_EQ(client_shared, server_shared) << "Shared secrets must match";

    // Arrange - 创建加密器
    AESCrypt client_cipher(client_shared);
    AESCrypt server_cipher(server_shared);
    std::string message = "Confidential log data";

    // Act - 客户端加密
    std::string encrypted;
    client_cipher.Encrypt(message.data(), message.size(), encrypted);

    // Act - 服务端解密
    std::string decrypted = server_cipher.Decrypt(encrypted.data(), encrypted.size());

    // Assert
    EXPECT_EQ(message, decrypted) << "Server should decrypt client's message correctly";
}

// 测试: 多轮通信
TEST_F(ECDHAESIntegrationTest, MultipleRoundTripCommunication) {
    // Arrange
    auto [client_pri, client_pub] = GenerateECDHkeyPair();
    auto [server_pri, server_pub] = GenerateECDHkeyPair();

    std::string shared_secret = ComputeECDHSharedSecret(client_pri, server_pub);
    AESCrypt cipher(shared_secret);

    // Act & Assert - 多次加密解密
    std::vector<std::string> messages = {
            "Message 1", "Another message", "Yet another message with more data", std::string(1000, 'X')  // 大消息
    };

    for (const auto& msg : messages) {
        std::string encrypted;
        cipher.Encrypt(msg.data(), msg.size(), encrypted);
        std::string decrypted = cipher.Decrypt(encrypted.data(), encrypted.size());
        EXPECT_EQ(msg, decrypted) << "Failed for message: " << msg.substr(0, 20);
    }
}

// 测试: 密钥的十六进制存储和恢复
TEST_F(ECDHAESIntegrationTest, KeyStorageAndRecovery) {
    // Arrange - 生成并存储密钥（模拟配置文件）
    auto [server_pri, server_pub] = GenerateECDHkeyPair();
    std::string server_pub_hex = BinaryKeyToHex(server_pub);

    // 模拟客户端从配置读取服务器公钥
    std::string restored_server_pub = HexKeyToBinary(server_pub_hex);

    // Act - 使用恢复的公钥
    auto [client_pri, client_pub] = GenerateECDHkeyPair();
    std::string shared_secret = ComputeECDHSharedSecret(client_pri, restored_server_pub);

    AESCrypt cipher(shared_secret);
    std::string message = "Test";
    std::string encrypted;
    cipher.Encrypt(message.data(), message.size(), encrypted);
    std::string decrypted = cipher.Decrypt(encrypted.data(), encrypted.size());

    // Assert
    EXPECT_EQ(message, decrypted) << "Should work with stored and restored keys";
}

// ============================================================================
// 性能测试（可选）
// ============================================================================

class AESPerformanceTest : public ::testing::Test {};

// 测试: 大数据加密性能
TEST_F(AESPerformanceTest, EncryptLargeData) {
    // Arrange
    AESCrypt cipher(std::string(32, 'K'));
    std::string large_data(1024 * 1024, 'A');  // 1MB 数据
    std::string encrypted;

    // Act
    cipher.Encrypt(large_data.data(), large_data.size(), encrypted);
    std::string decrypted = cipher.Decrypt(encrypted.data(), encrypted.size());

    // Assert
    EXPECT_EQ(large_data, decrypted) << "Large data encryption should work";
    // 注意: 实际性能测试应该使用 Google Benchmark 或类似工具
}
