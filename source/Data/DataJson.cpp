#include "MpaUtils/Data/DataJson.h"

#include <cmath>
#include <sstream>
#include <stdexcept>

#include "MpaUtils/Core/JsonValue.h"
#include "MpaUtils/Core/RuntimeValue.h"

namespace mpa::data {

namespace {

std::string NumberToString(double value) {
    const auto rounded = std::round(value);
    if (std::fabs(value - rounded) < 0.0000001) {
        return std::to_string(static_cast<long long>(rounded));
    }
    std::ostringstream stream;
    stream << value;
    return stream.str();
}

std::string ScalarJsonToString(const JsonValue& value) {
    if (value.IsString()) {
        return value.AsString();
    }
    if (value.IsNumber()) {
        return NumberToString(value.AsNumber());
    }
    if (value.IsBool()) {
        return value.AsBool() ? "true" : "false";
    }
    if (std::holds_alternative<std::nullptr_t>(value.storage)) {
        return {};
    }
    throw std::runtime_error("JsonParseToContext: nested JSON values are not supported");
}

std::string JsonStringifyString(std::string_view value) {
    std::string out = "\"";
    out.append(JsonEscape(value));
    out.push_back('"');
    return out;
}

}  // namespace

void JsonParseToContext(std::string_view json_text,
                        const std::string& result_key,
                        ExecutionContext& context) {
    const auto root = JsonParser(json_text).Parse();
    if (root.IsObject()) {
        context.CreateDictionary(result_key);
        for (const auto& [key, value] : root.AsObject()) {
            context.SetDictionaryValue(result_key, key, ScalarJsonToString(value));
        }
        return;
    }

    if (root.IsArray()) {
        context.CreateList(result_key);
        for (const auto& value : root.AsArray()) {
            context.AppendListItem(result_key, ScalarJsonToString(value));
        }
        return;
    }

    context.SetString(result_key, ScalarJsonToString(root));
}

std::string JsonStringifyContextValue(const std::string& source_var,
                                      const ExecutionContext& context) {
    const auto it = context.values.find(source_var);
    if (it == context.values.end()) {
        throw std::runtime_error("JsonStringifyContextValue: context value not found: " + source_var);
    }

    const auto& value = it->second;
    if (value.IsString()) {
        return JsonStringifyString(value.AsString());
    }
    if (value.IsList()) {
        std::string out = "[";
        const auto& list = value.AsList();
        for (size_t i = 0; i < list.size(); ++i) {
            if (i > 0) {
                out.push_back(',');
            }
            out.append(JsonStringifyString(list[i]));
        }
        out.push_back(']');
        return out;
    }
    if (value.IsDictionary()) {
        std::string out = "{";
        bool first = true;
        for (const auto& [key, entry] : value.AsDictionary()) {
            if (!first) {
                out.push_back(',');
            }
            first = false;
            out.append(JsonStringifyString(key));
            out.push_back(':');
            out.append(JsonStringifyString(entry));
        }
        out.push_back('}');
        return out;
    }

    throw std::runtime_error("JsonStringifyContextValue: unsupported value kind");
}

}  // namespace mpa::data

