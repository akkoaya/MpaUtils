#pragma once

#include <string>
#include <string_view>

namespace mpa {
struct ExecutionContext;
}

namespace mpa::data {

void JsonParseToContext(std::string_view json_text,
                        const std::string& result_key,
                        ExecutionContext& context);

std::string JsonStringifyContextValue(const std::string& source_var,
                                      const ExecutionContext& context);

}  // namespace mpa::data
