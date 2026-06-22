#pragma once

#include <string>
#include <string_view>

namespace mpa::data {

enum class CryptoHashAlgorithm { Md5, Sha1, Sha256, Sha512 };
enum class CryptoHmacAlgorithm { Sha1, Sha256, Sha512 };

std::string CryptoHash(std::string_view text, CryptoHashAlgorithm algorithm);
std::string CryptoHmac(
    std::string_view text,
    std::string_view key,
    CryptoHmacAlgorithm algorithm);
std::string CryptoAes256CbcEncrypt(
    std::string_view text,
    std::string_view key,
    std::string_view iv);
std::string CryptoAes256CbcDecrypt(
    std::string_view base64_ciphertext,
    std::string_view key,
    std::string_view iv);

}  // namespace mpa::data
