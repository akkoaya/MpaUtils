#pragma once

#include <string>
#include <string_view>

namespace mpa::data {

std::string Base64Encode(std::string_view text);
std::string Base64Decode(std::string_view text);

}  // namespace mpa::data
