#include "MpaUtils/Data/DataCrypto.h"

#include "MpaUtils/SafeWindows.hpp"
#include <bcrypt.h>

#include <array>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "MpaUtils/Data/DataBase64.h"

namespace mpa::data {

namespace {

struct AlgorithmHandle {
    BCRYPT_ALG_HANDLE value = nullptr;

    AlgorithmHandle() = default;
    AlgorithmHandle(const AlgorithmHandle&) = delete;
    AlgorithmHandle& operator=(const AlgorithmHandle&) = delete;
    AlgorithmHandle(AlgorithmHandle&& other) noexcept : value(other.value) {
        other.value = nullptr;
    }
    AlgorithmHandle& operator=(AlgorithmHandle&& other) noexcept {
        if (this != &other) {
            if (value != nullptr) {
                BCryptCloseAlgorithmProvider(value, 0);
            }
            value = other.value;
            other.value = nullptr;
        }
        return *this;
    }

    ~AlgorithmHandle() {
        if (value != nullptr) {
            BCryptCloseAlgorithmProvider(value, 0);
        }
    }
};

struct HashHandle {
    BCRYPT_HASH_HANDLE value = nullptr;

    HashHandle() = default;
    HashHandle(const HashHandle&) = delete;
    HashHandle& operator=(const HashHandle&) = delete;
    HashHandle(HashHandle&& other) noexcept : value(other.value) {
        other.value = nullptr;
    }
    HashHandle& operator=(HashHandle&& other) noexcept {
        if (this != &other) {
            if (value != nullptr) {
                BCryptDestroyHash(value);
            }
            value = other.value;
            other.value = nullptr;
        }
        return *this;
    }

    ~HashHandle() {
        if (value != nullptr) {
            BCryptDestroyHash(value);
        }
    }
};

struct KeyHandle {
    BCRYPT_KEY_HANDLE value = nullptr;

    KeyHandle() = default;
    KeyHandle(const KeyHandle&) = delete;
    KeyHandle& operator=(const KeyHandle&) = delete;
    KeyHandle(KeyHandle&& other) noexcept : value(other.value) {
        other.value = nullptr;
    }
    KeyHandle& operator=(KeyHandle&& other) noexcept {
        if (this != &other) {
            if (value != nullptr) {
                BCryptDestroyKey(value);
            }
            value = other.value;
            other.value = nullptr;
        }
        return *this;
    }

    ~KeyHandle() {
        if (value != nullptr) {
            BCryptDestroyKey(value);
        }
    }
};

[[noreturn]] void ThrowCryptoError(const std::string& message) {
    throw std::runtime_error("data.crypto: " + message);
}

void CheckStatus(NTSTATUS status, const std::string& message) {
    if (!BCRYPT_SUCCESS(status)) {
        ThrowCryptoError(message);
    }
}

std::vector<unsigned char> Bytes(std::string_view text) {
    return std::vector<unsigned char>(text.begin(), text.end());
}

std::string HexEncode(const std::vector<unsigned char>& bytes) {
    static constexpr std::array<char, 16> kHex = {
        '0', '1', '2', '3', '4', '5', '6', '7',
        '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
    std::string out;
    out.reserve(bytes.size() * 2);
    for (const auto byte : bytes) {
        out.push_back(kHex[(byte >> 4) & 0x0F]);
        out.push_back(kHex[byte & 0x0F]);
    }
    return out;
}

const wchar_t* HashAlgorithmName(CryptoHashAlgorithm algorithm) {
    switch (algorithm) {
        case CryptoHashAlgorithm::Md5: return BCRYPT_MD5_ALGORITHM;
        case CryptoHashAlgorithm::Sha1: return BCRYPT_SHA1_ALGORITHM;
        case CryptoHashAlgorithm::Sha256: return BCRYPT_SHA256_ALGORITHM;
        case CryptoHashAlgorithm::Sha512: return BCRYPT_SHA512_ALGORITHM;
    }
    ThrowCryptoError("invalid hash algorithm");
}

const wchar_t* HmacAlgorithmName(CryptoHmacAlgorithm algorithm) {
    switch (algorithm) {
        case CryptoHmacAlgorithm::Sha1: return BCRYPT_SHA1_ALGORITHM;
        case CryptoHmacAlgorithm::Sha256: return BCRYPT_SHA256_ALGORITHM;
        case CryptoHmacAlgorithm::Sha512: return BCRYPT_SHA512_ALGORITHM;
    }
    ThrowCryptoError("invalid hmac algorithm");
}

DWORD GetDwordProperty(BCRYPT_ALG_HANDLE algorithm, const wchar_t* property) {
    DWORD value = 0;
    DWORD written = 0;
    CheckStatus(
        BCryptGetProperty(
            algorithm,
            property,
            reinterpret_cast<PUCHAR>(&value),
            sizeof(value),
            &written,
            0),
        "failed to query algorithm property");
    return value;
}

std::vector<unsigned char> ComputeHash(
    const wchar_t* algorithm_name,
    std::string_view text,
    std::string_view key,
    bool hmac) {
    AlgorithmHandle algorithm;
    CheckStatus(
        BCryptOpenAlgorithmProvider(
            &algorithm.value,
            algorithm_name,
            nullptr,
            hmac ? BCRYPT_ALG_HANDLE_HMAC_FLAG : 0),
        "failed to open hash algorithm");

    const auto object_length = GetDwordProperty(algorithm.value, BCRYPT_OBJECT_LENGTH);
    const auto hash_length = GetDwordProperty(algorithm.value, BCRYPT_HASH_LENGTH);
    std::vector<unsigned char> object(object_length);
    std::vector<unsigned char> key_bytes = Bytes(key);

    HashHandle hash;
    CheckStatus(
        BCryptCreateHash(
            algorithm.value,
            &hash.value,
            object.data(),
            static_cast<ULONG>(object.size()),
            hmac && !key_bytes.empty() ? key_bytes.data() : nullptr,
            hmac ? static_cast<ULONG>(key_bytes.size()) : 0,
            0),
        "failed to create hash");

    auto text_bytes = Bytes(text);
    CheckStatus(
        BCryptHashData(
            hash.value,
            text_bytes.empty() ? nullptr : text_bytes.data(),
            static_cast<ULONG>(text_bytes.size()),
            0),
        "failed to hash data");

    std::vector<unsigned char> output(hash_length);
    CheckStatus(
        BCryptFinishHash(hash.value, output.data(), static_cast<ULONG>(output.size()), 0),
        "failed to finish hash");
    return output;
}

void ValidateAesKeyAndIv(std::string_view key, std::string_view iv) {
    if (key.size() != 32) {
        throw std::runtime_error("data.crypto.aes: key must be exactly 32 bytes");
    }
    if (iv.size() != 16) {
        throw std::runtime_error("data.crypto.aes: iv must be exactly 16 bytes");
    }
}

AlgorithmHandle OpenAesAlgorithm() {
    AlgorithmHandle algorithm;
    CheckStatus(
        BCryptOpenAlgorithmProvider(&algorithm.value, BCRYPT_AES_ALGORITHM, nullptr, 0),
        "failed to open AES algorithm");
    CheckStatus(
        BCryptSetProperty(
            algorithm.value,
            BCRYPT_CHAINING_MODE,
            reinterpret_cast<PUCHAR>(const_cast<wchar_t*>(BCRYPT_CHAIN_MODE_CBC)),
            static_cast<ULONG>((wcslen(BCRYPT_CHAIN_MODE_CBC) + 1) * sizeof(wchar_t)),
            0),
        "failed to set AES CBC mode");
    return algorithm;
}

KeyHandle GenerateAesKey(BCRYPT_ALG_HANDLE algorithm, std::string_view key) {
    auto key_bytes = Bytes(key);
    KeyHandle key_handle;
    CheckStatus(
        BCryptGenerateSymmetricKey(
            algorithm,
            &key_handle.value,
            nullptr,
            0,
            key_bytes.data(),
            static_cast<ULONG>(key_bytes.size()),
            0),
        "failed to generate AES key");
    return key_handle;
}

std::vector<unsigned char> AesCrypt(
    std::string_view input,
    std::string_view key,
    std::string_view iv,
    bool encrypt) {
    ValidateAesKeyAndIv(key, iv);
    auto algorithm = OpenAesAlgorithm();
    auto key_handle = GenerateAesKey(algorithm.value, key);
    auto input_bytes = Bytes(input);
    auto iv_bytes = Bytes(iv);

    ULONG output_size = 0;
    auto status = encrypt
        ? BCryptEncrypt(
              key_handle.value,
              input_bytes.empty() ? nullptr : input_bytes.data(),
              static_cast<ULONG>(input_bytes.size()),
              nullptr,
              iv_bytes.data(),
              static_cast<ULONG>(iv_bytes.size()),
              nullptr,
              0,
              &output_size,
              BCRYPT_BLOCK_PADDING)
        : BCryptDecrypt(
              key_handle.value,
              input_bytes.empty() ? nullptr : input_bytes.data(),
              static_cast<ULONG>(input_bytes.size()),
              nullptr,
              iv_bytes.data(),
              static_cast<ULONG>(iv_bytes.size()),
              nullptr,
              0,
              &output_size,
              BCRYPT_BLOCK_PADDING);
    CheckStatus(status, encrypt ? "failed to size AES ciphertext" : "failed to size AES plaintext");

    std::vector<unsigned char> output(output_size);
    iv_bytes = Bytes(iv);
    status = encrypt
        ? BCryptEncrypt(
              key_handle.value,
              input_bytes.empty() ? nullptr : input_bytes.data(),
              static_cast<ULONG>(input_bytes.size()),
              nullptr,
              iv_bytes.data(),
              static_cast<ULONG>(iv_bytes.size()),
              output.data(),
              static_cast<ULONG>(output.size()),
              &output_size,
              BCRYPT_BLOCK_PADDING)
        : BCryptDecrypt(
              key_handle.value,
              input_bytes.empty() ? nullptr : input_bytes.data(),
              static_cast<ULONG>(input_bytes.size()),
              nullptr,
              iv_bytes.data(),
              static_cast<ULONG>(iv_bytes.size()),
              output.data(),
              static_cast<ULONG>(output.size()),
              &output_size,
              BCRYPT_BLOCK_PADDING);
    CheckStatus(status, encrypt ? "failed to encrypt AES data" : "failed to decrypt AES data");
    output.resize(output_size);
    return output;
}

}  // namespace

std::string CryptoHash(std::string_view text, CryptoHashAlgorithm algorithm) {
    return HexEncode(ComputeHash(HashAlgorithmName(algorithm), text, {}, false));
}

std::string CryptoHmac(
    std::string_view text,
    std::string_view key,
    CryptoHmacAlgorithm algorithm) {
    return HexEncode(ComputeHash(HmacAlgorithmName(algorithm), text, key, true));
}

std::string CryptoAes256CbcEncrypt(
    std::string_view text,
    std::string_view key,
    std::string_view iv) {
    const auto ciphertext = AesCrypt(text, key, iv, true);
    return Base64Encode(std::string_view(
        reinterpret_cast<const char*>(ciphertext.data()),
        ciphertext.size()));
}

std::string CryptoAes256CbcDecrypt(
    std::string_view base64_ciphertext,
    std::string_view key,
    std::string_view iv) {
    const auto ciphertext = Base64Decode(base64_ciphertext);
    const auto plaintext = AesCrypt(ciphertext, key, iv, false);
    return std::string(plaintext.begin(), plaintext.end());
}

}  // namespace mpa::data

