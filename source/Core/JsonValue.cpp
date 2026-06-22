#include "MpaUtils/Core/JsonValue.h"

#include <cmath>
#include <cctype>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace mpa {

bool JsonValue::IsObject() const { return std::holds_alternative<Object>(storage); }
bool JsonValue::IsArray() const { return std::holds_alternative<Array>(storage); }
bool JsonValue::IsString() const { return std::holds_alternative<std::string>(storage); }
bool JsonValue::IsNumber() const { return std::holds_alternative<double>(storage); }
bool JsonValue::IsBool() const { return std::holds_alternative<bool>(storage); }
const JsonValue::Object& JsonValue::AsObject() const { return std::get<Object>(storage); }
const JsonValue::Array& JsonValue::AsArray() const { return std::get<Array>(storage); }
const std::string& JsonValue::AsString() const { return std::get<std::string>(storage); }
double JsonValue::AsNumber() const { return std::get<double>(storage); }
bool JsonValue::AsBool() const { return std::get<bool>(storage); }

JsonParser::JsonParser(std::string_view text) : text_(text) {}

JsonValue JsonParser::Parse() {
    SkipWhitespace();
    auto value = ParseValue();
    SkipWhitespace();
    if (offset_ != text_.size()) {
        throw std::runtime_error("unexpected trailing content");
    }
    return value;
}

char JsonParser::Peek() const {
    return offset_ < text_.size() ? text_[offset_] : '\0';
}

char JsonParser::Consume() {
    if (offset_ >= text_.size()) {
        throw std::runtime_error("unexpected end of input");
    }
    return text_[offset_++];
}

void JsonParser::SkipWhitespace() {
    while (offset_ < text_.size() &&
           std::isspace(static_cast<unsigned char>(text_[offset_])) != 0) {
        ++offset_;
    }
}

void JsonParser::Expect(char expected) {
    const auto actual = Consume();
    if (actual != expected) {
        throw std::runtime_error("unexpected token");
    }
}

JsonValue JsonParser::ParseValue() {
    SkipWhitespace();
    const auto next = Peek();
    if (next == '{') {
        return ParseObject();
    }
    if (next == '[') {
        return ParseArray();
    }
    if (next == '"') {
        return JsonValue{ParseString()};
    }
    if (next == '-' || std::isdigit(static_cast<unsigned char>(next)) != 0) {
        return JsonValue{ParseNumber()};
    }
    if (MatchKeyword("true")) {
        return JsonValue{true};
    }
    if (MatchKeyword("false")) {
        return JsonValue{false};
    }
    if (MatchKeyword("null")) {
        return JsonValue{nullptr};
    }
    throw std::runtime_error("unsupported json value");
}

JsonValue JsonParser::ParseObject() {
    Expect('{');
    JsonValue::Object object;
    SkipWhitespace();
    if (Peek() == '}') {
        Consume();
        return JsonValue{object};
    }

    while (true) {
        SkipWhitespace();
        const auto key = ParseString();
        SkipWhitespace();
        Expect(':');
        SkipWhitespace();
        object.emplace(key, ParseValue());
        SkipWhitespace();
        const auto separator = Consume();
        if (separator == '}') {
            break;
        }
        if (separator != ',') {
            throw std::runtime_error("expected object separator");
        }
    }

    return JsonValue{object};
}

JsonValue JsonParser::ParseArray() {
    Expect('[');
    JsonValue::Array array;
    SkipWhitespace();
    if (Peek() == ']') {
        Consume();
        return JsonValue{array};
    }

    while (true) {
        SkipWhitespace();
        array.push_back(ParseValue());
        SkipWhitespace();
        const auto separator = Consume();
        if (separator == ']') {
            break;
        }
        if (separator != ',') {
            throw std::runtime_error("expected array separator");
        }
    }

    return JsonValue{array};
}

std::string JsonParser::ParseString() {
    Expect('"');
    std::string result;
    while (true) {
        const auto ch = Consume();
        if (ch == '"') {
            break;
        }
        if (ch == '\\') {
            const auto escaped = Consume();
            switch (escaped) {
                case '"':
                case '\\':
                case '/':
                    result.push_back(escaped);
                    break;
                case 'b':
                    result.push_back('\b');
                    break;
                case 'f':
                    result.push_back('\f');
                    break;
                case 'n':
                    result.push_back('\n');
                    break;
                case 'r':
                    result.push_back('\r');
                    break;
                case 't':
                    result.push_back('\t');
                    break;
                default:
                    throw std::runtime_error("unsupported escape sequence");
            }
            continue;
        }
        result.push_back(ch);
    }
    return result;
}

double JsonParser::ParseNumber() {
    const auto start = offset_;
    if (Peek() == '-') {
        Consume();
    }
    while (std::isdigit(static_cast<unsigned char>(Peek())) != 0) {
        Consume();
    }
    if (Peek() == '.') {
        Consume();
        while (std::isdigit(static_cast<unsigned char>(Peek())) != 0) {
            Consume();
        }
    }
    if (Peek() == 'e' || Peek() == 'E') {
        Consume();
        if (Peek() == '+' || Peek() == '-') {
            Consume();
        }
        const auto exponent_start = offset_;
        while (std::isdigit(static_cast<unsigned char>(Peek())) != 0) {
            Consume();
        }
        if (offset_ == exponent_start) {
            throw std::runtime_error("invalid number exponent");
        }
    }
    return std::stod(std::string(text_.substr(start, offset_ - start)));
}

bool JsonParser::MatchKeyword(std::string_view keyword) {
    if (text_.substr(offset_, keyword.size()) != keyword) {
        return false;
    }
    offset_ += keyword.size();
    return true;
}

std::string JsonEscape(std::string_view value) {
    std::string escaped;
    escaped.reserve(value.size() + 4);
    for (const auto ch : value) {
        switch (ch) {
            case '\\':
                escaped += "\\\\";
                break;
            case '"':
                escaped += "\\\"";
                break;
            case '\n':
                escaped += "\\n";
                break;
            case '\r':
                escaped += "\\r";
                break;
            case '\t':
                escaped += "\\t";
                break;
            default:
                escaped.push_back(ch);
                break;
        }
    }
    return escaped;
}

namespace {

std::string JsonSerializeObject(const JsonValue::Object& object);
std::string JsonSerializeArray(const JsonValue::Array& array);

std::string JsonSerializeValue(const JsonValue& value) {
    if (std::holds_alternative<std::nullptr_t>(value.storage)) {
        return "null";
    }
    if (value.IsBool()) {
        return value.AsBool() ? "true" : "false";
    }
    if (value.IsNumber()) {
        const auto number = value.AsNumber();
        if (std::isfinite(number) && std::floor(number) == number) {
            std::ostringstream integer_stream;
            integer_stream << std::fixed << std::setprecision(0) << number;
            return integer_stream.str();
        }
        std::ostringstream stream;
        stream << number;
        return stream.str();
    }
    if (value.IsString()) {
        return "\"" + JsonEscape(value.AsString()) + "\"";
    }
    if (value.IsArray()) {
        return JsonSerializeArray(value.AsArray());
    }
    return JsonSerializeObject(value.AsObject());
}

std::string JsonSerializeObject(const JsonValue::Object& object) {
    std::ostringstream stream;
    stream << "{";
    bool first = true;
    for (const auto& [key, value] : object) {
        if (!first) {
            stream << ",";
        }
        first = false;
        stream << "\"" << JsonEscape(key) << "\":" << JsonSerializeValue(value);
    }
    stream << "}";
    return stream.str();
}

std::string JsonSerializeArray(const JsonValue::Array& array) {
    std::ostringstream stream;
    stream << "[";
    for (size_t index = 0; index < array.size(); ++index) {
        if (index > 0) {
            stream << ",";
        }
        stream << JsonSerializeValue(array[index]);
    }
    stream << "]";
    return stream.str();
}

}  // namespace

std::string JsonSerialize(const JsonValue& value) { return JsonSerializeValue(value); }

JsonValue JsonParseWithComments(std::string_view text) {
    std::string stripped;
    stripped.reserve(text.size());

    bool in_string = false;
    bool escaped = false;
    for (size_t index = 0; index < text.size(); ++index) {
        const char ch = text[index];
        if (in_string) {
            stripped.push_back(ch);
            if (escaped) {
                escaped = false;
            } else if (ch == '\\') {
                escaped = true;
            } else if (ch == '"') {
                in_string = false;
            }
            continue;
        }

        if (ch == '"') {
            in_string = true;
            stripped.push_back(ch);
            continue;
        }

        if (ch == '/' && index + 1 < text.size()) {
            const char next = text[index + 1];
            if (next == '/') {
                stripped.push_back(' ');
                stripped.push_back(' ');
                index += 2;
                while (index < text.size() && text[index] != '\n' && text[index] != '\r') {
                    stripped.push_back(' ');
                    ++index;
                }
                if (index < text.size()) {
                    stripped.push_back(text[index]);
                }
                continue;
            }
            if (next == '*') {
                stripped.push_back(' ');
                stripped.push_back(' ');
                index += 2;
                bool closed = false;
                while (index < text.size()) {
                    const char comment_ch = text[index];
                    if (comment_ch == '*' && index + 1 < text.size() && text[index + 1] == '/') {
                        stripped.push_back(' ');
                        stripped.push_back(' ');
                        ++index;
                        closed = true;
                        break;
                    }
                    stripped.push_back((comment_ch == '\n' || comment_ch == '\r') ? comment_ch : ' ');
                    ++index;
                }
                if (!closed) {
                    throw std::runtime_error("unterminated json comment");
                }
                continue;
            }
        }

        stripped.push_back(ch);
    }

    return JsonParser(stripped).Parse();
}

}  // namespace mpa
