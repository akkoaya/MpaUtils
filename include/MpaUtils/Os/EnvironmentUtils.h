#pragma once

#include <cstdlib>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace mpa {

inline std::optional<std::string> ReadEnvironmentVariable(std::string_view name) {
    const std::string env_name(name);
#if defined(_WIN32)
    char* value = nullptr;
    size_t size = 0;
    if (_dupenv_s(&value, &size, env_name.c_str()) != 0 || value == nullptr) {
        return std::nullopt;
    }

    std::unique_ptr<char, decltype(&std::free)> owned_value(value, &std::free);
    if (size <= 1 || owned_value.get()[0] == '\0') {
        return std::nullopt;
    }
    return std::string(owned_value.get());
#else
    if (const char* value = std::getenv(env_name.c_str()); value != nullptr && value[0] != '\0') {
        return std::string(value);
    }
    return std::nullopt;
#endif
}

inline std::optional<std::filesystem::path> ReadEnvironmentPath(std::string_view name) {
    if (const auto value = ReadEnvironmentVariable(name); value.has_value()) {
        return std::filesystem::path(*value);
    }
    return std::nullopt;
}

}  // namespace mpa
