#include "MpaUtils/Data/DataBase64.h"

#include <array>
#include <cctype>
#include <stdexcept>

namespace mpa::data {

namespace {

constexpr char kAlphabet[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

int DecodeValue(unsigned char ch) {
    if (ch >= 'A' && ch <= 'Z') return ch - 'A';
    if (ch >= 'a' && ch <= 'z') return ch - 'a' + 26;
    if (ch >= '0' && ch <= '9') return ch - '0' + 52;
    if (ch == '+') return 62;
    if (ch == '/') return 63;
    return -1;
}

}  // namespace

std::string Base64Encode(std::string_view text) {
    std::string out;
    out.reserve(((text.size() + 2) / 3) * 4);

    size_t i = 0;
    while (i < text.size()) {
        const auto b0 = static_cast<unsigned char>(text[i++]);
        const bool has_b1 = i < text.size();
        const auto b1 = has_b1 ? static_cast<unsigned char>(text[i++]) : 0U;
        const bool has_b2 = i < text.size();
        const auto b2 = has_b2 ? static_cast<unsigned char>(text[i++]) : 0U;

        out.push_back(kAlphabet[(b0 >> 2) & 0x3F]);
        out.push_back(kAlphabet[((b0 & 0x03) << 4) | ((b1 >> 4) & 0x0F)]);
        out.push_back(has_b1 ? kAlphabet[((b1 & 0x0F) << 2) | ((b2 >> 6) & 0x03)] : '=');
        out.push_back(has_b2 ? kAlphabet[b2 & 0x3F] : '=');
    }

    return out;
}

std::string Base64Decode(std::string_view text) {
    std::string compact;
    compact.reserve(text.size());
    for (const auto ch : text) {
        if (std::isspace(static_cast<unsigned char>(ch)) != 0) {
            continue;
        }
        compact.push_back(ch);
    }

    if (compact.empty()) {
        return {};
    }
    if (compact.size() % 4 != 0) {
        throw std::runtime_error("Base64Decode: input length must be a multiple of 4");
    }

    std::string out;
    out.reserve((compact.size() / 4) * 3);
    for (size_t i = 0; i < compact.size(); i += 4) {
        std::array<int, 4> values{};
        int padding = 0;
        for (size_t k = 0; k < 4; ++k) {
            const auto ch = static_cast<unsigned char>(compact[i + k]);
            if (ch == '=') {
                if (k < 2) {
                    throw std::runtime_error("Base64Decode: invalid padding");
                }
                values[k] = 0;
                ++padding;
                continue;
            }
            if (padding != 0) {
                throw std::runtime_error("Base64Decode: data after padding");
            }
            values[k] = DecodeValue(ch);
            if (values[k] < 0) {
                throw std::runtime_error("Base64Decode: invalid character");
            }
        }
        if (padding > 2) {
            throw std::runtime_error("Base64Decode: invalid padding");
        }
        if (padding != 0 && i + 4 != compact.size()) {
            throw std::runtime_error("Base64Decode: padding before final block");
        }

        out.push_back(static_cast<char>((values[0] << 2) | (values[1] >> 4)));
        if (padding < 2) {
            out.push_back(static_cast<char>(((values[1] & 0x0F) << 4) | (values[2] >> 2)));
        }
        if (padding < 1) {
            out.push_back(static_cast<char>(((values[2] & 0x03) << 6) | values[3]));
        }
    }

    return out;
}

}  // namespace mpa::data

