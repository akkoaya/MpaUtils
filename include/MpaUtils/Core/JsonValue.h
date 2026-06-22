#pragma once

#include <map>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace mpa {

struct JsonValue {
    using Object = std::map<std::string, JsonValue>;
    using Array = std::vector<JsonValue>;
    using Storage =
        std::variant<std::nullptr_t, bool, double, std::string, Object, Array>;

    Storage storage;

    [[nodiscard]] bool IsObject() const;
    [[nodiscard]] bool IsArray() const;
    [[nodiscard]] bool IsString() const;
    [[nodiscard]] bool IsNumber() const;
    [[nodiscard]] bool IsBool() const;
    [[nodiscard]] const Object& AsObject() const;
    [[nodiscard]] const Array& AsArray() const;
    [[nodiscard]] const std::string& AsString() const;
    [[nodiscard]] double AsNumber() const;
    [[nodiscard]] bool AsBool() const;
};

class JsonParser {
public:
    explicit JsonParser(std::string_view text);

    JsonValue Parse();

private:
    [[nodiscard]] char Peek() const;
    char Consume();
    void SkipWhitespace();
    void Expect(char expected);
    JsonValue ParseValue();
    JsonValue ParseObject();
    JsonValue ParseArray();
    std::string ParseString();
    double ParseNumber();
    bool MatchKeyword(std::string_view keyword);

    std::string_view text_;
    size_t offset_ = 0;
};

std::string JsonEscape(std::string_view value);
std::string JsonSerialize(const JsonValue& value);
JsonValue JsonParseWithComments(std::string_view text);

}  // namespace mpa
