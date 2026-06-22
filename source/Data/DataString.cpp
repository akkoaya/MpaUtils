#include "MpaUtils/Data/DataString.h"

#include <cstring>
#include <regex>
#include <stdexcept>

namespace mpa::data {

namespace {

// Counts the leading UTF-8 byte's encoded codepoint length.
// Returns 1 for ASCII / continuation bytes / malformed leading bytes so iteration
// always advances.
size_t Utf8LeadByteWidth(unsigned char byte) {
    if ((byte & 0x80U) == 0x00U) return 1;          // 0xxxxxxx
    if ((byte & 0xE0U) == 0xC0U) return 2;          // 110xxxxx
    if ((byte & 0xF0U) == 0xE0U) return 3;          // 1110xxxx
    if ((byte & 0xF8U) == 0xF0U) return 4;          // 11110xxx
    return 1;                                       // continuation / malformed
}

}  // namespace

size_t StringLength(std::string_view text) {
    size_t count = 0;
    size_t i = 0;
    while (i < text.size()) {
        const auto width = Utf8LeadByteWidth(static_cast<unsigned char>(text[i]));
        i += (i + width <= text.size()) ? width : 1;
        ++count;
    }
    return count;
}

namespace {

bool IsTrimByte(unsigned char byte) {
    return byte == ' ' || byte == '\t' || byte == '\n' || byte == '\r' || byte == '\v'
        || byte == '\f';
}

// True if the 3 bytes starting at i encode U+3000 (E3 80 80).
bool IsIdeographicSpaceAt(std::string_view text, size_t i) {
    return i + 3 <= text.size() && static_cast<unsigned char>(text[i]) == 0xE3
        && static_cast<unsigned char>(text[i + 1]) == 0x80
        && static_cast<unsigned char>(text[i + 2]) == 0x80;
}

size_t LeftTrimIndex(std::string_view text) {
    size_t i = 0;
    while (i < text.size()) {
        if (IsTrimByte(static_cast<unsigned char>(text[i]))) { ++i; continue; }
        if (IsIdeographicSpaceAt(text, i)) { i += 3; continue; }
        break;
    }
    return i;
}

size_t RightTrimEnd(std::string_view text) {
    size_t end = text.size();
    while (end > 0) {
        if (IsTrimByte(static_cast<unsigned char>(text[end - 1]))) { --end; continue; }
        if (end >= 3 && IsIdeographicSpaceAt(text, end - 3)) { end -= 3; continue; }
        break;
    }
    return end;
}

}  // namespace

std::string StringTrim(std::string_view text, TrimSide side) {
    const size_t begin = (side == TrimSide::Right) ? 0 : LeftTrimIndex(text);
    const size_t end = (side == TrimSide::Left) ? text.size() : RightTrimEnd(text);
    if (begin >= end) return {};
    return std::string(text.substr(begin, end - begin));
}

namespace {

bool IsAsciiLetter(unsigned char byte) {
    return (byte >= 'A' && byte <= 'Z') || (byte >= 'a' && byte <= 'z');
}

unsigned char ToAsciiUpper(unsigned char byte) {
    return (byte >= 'a' && byte <= 'z') ? static_cast<unsigned char>(byte - 'a' + 'A') : byte;
}

unsigned char ToAsciiLower(unsigned char byte) {
    return (byte >= 'A' && byte <= 'Z') ? static_cast<unsigned char>(byte - 'A' + 'a') : byte;
}

}  // namespace

std::string StringChangeCase(std::string_view text, CaseMode mode) {
    std::string out;
    out.reserve(text.size());
    bool prev_was_letter = false;
    for (size_t i = 0; i < text.size(); ++i) {
        const auto byte = static_cast<unsigned char>(text[i]);
        if (byte >= 0x80) {
            out.push_back(static_cast<char>(byte));
            prev_was_letter = false;
            continue;
        }
        if (mode == CaseMode::Upper) {
            out.push_back(static_cast<char>(ToAsciiUpper(byte)));
        } else if (mode == CaseMode::Lower) {
            out.push_back(static_cast<char>(ToAsciiLower(byte)));
        } else {  // Title
            if (IsAsciiLetter(byte)) {
                out.push_back(
                    prev_was_letter ? static_cast<char>(ToAsciiLower(byte))
                                    : static_cast<char>(ToAsciiUpper(byte)));
            } else {
                out.push_back(static_cast<char>(byte));
            }
        }
        prev_was_letter = IsAsciiLetter(byte);
    }
    return out;
}

std::string StringAppend(std::string_view base, std::string_view suffix, bool newline) {
    std::string out;
    out.reserve(base.size() + suffix.size() + (newline ? 1 : 0));
    out.append(base);
    if (newline) out.push_back('\n');
    out.append(suffix);
    return out;
}

namespace {

// Returns the byte offset after the i-th UTF-8 codepoint or text.size() if out of range.
size_t Utf8AdvanceCodepoints(std::string_view text, size_t start_byte, size_t codepoint_count) {
    size_t i = start_byte;
    for (size_t c = 0; c < codepoint_count && i < text.size(); ++c) {
        const auto width = Utf8LeadByteWidth(static_cast<unsigned char>(text[i]));
        i += (i + width <= text.size()) ? width : 1;
    }
    return i;
}

// Returns the substring containing the first `codepoint_count` codepoints of `text`.
std::string_view Utf8TakeCodepoints(std::string_view text, size_t codepoint_count) {
    const auto end_byte = Utf8AdvanceCodepoints(text, 0, codepoint_count);
    return text.substr(0, end_byte);
}

}  // namespace

std::string StringPad(std::string_view text,
                      std::string_view fill,
                      size_t total_codepoints,
                      PadSide side) {
    const size_t current = StringLength(text);
    if (current >= total_codepoints) {
        return std::string(text);
    }
    const size_t gap = total_codepoints - current;
    if (fill.empty()) {
        throw std::invalid_argument("StringPad: fill must not be empty when padding is required");
    }
    const size_t fill_len = StringLength(fill);

    std::string padding;
    padding.reserve(gap * fill.size());
    size_t produced = 0;
    while (produced < gap) {
        const size_t take = (fill_len < gap - produced) ? fill_len : (gap - produced);
        const auto slice = Utf8TakeCodepoints(fill, take);
        padding.append(slice);
        produced += take;
    }

    if (side == PadSide::Right) {
        std::string out(text);
        out.append(padding);
        return out;
    }
    std::string out = padding;
    out.append(text);
    return out;
}

std::string StringSubstring(std::string_view text, const SubstringOptions& opts) {
    if (opts.start_mode == SubstringOptions::StartMode::ByIndex && opts.start_index < 0) {
        throw std::invalid_argument("StringSubstring: start_index must be >= 0");
    }
    if (opts.length_mode == SubstringOptions::LengthMode::ByLength && opts.length_value < 0) {
        throw std::invalid_argument("StringSubstring: length_value must be >= 0");
    }

    size_t start_byte = 0;
    switch (opts.start_mode) {
        case SubstringOptions::StartMode::FromFirst:
            start_byte = 0;
            break;
        case SubstringOptions::StartMode::ByIndex:
            start_byte = Utf8AdvanceCodepoints(text, 0, static_cast<size_t>(opts.start_index));
            break;
        case SubstringOptions::StartMode::ByText: {
            const auto pos = text.find(opts.start_text);
            if (pos == std::string_view::npos) return {};
            start_byte = pos;
            break;
        }
    }

    if (start_byte >= text.size()) return {};

    if (opts.length_mode == SubstringOptions::LengthMode::ToEnd) {
        return std::string(text.substr(start_byte));
    }
    const auto end_byte =
        Utf8AdvanceCodepoints(text, start_byte, static_cast<size_t>(opts.length_value));
    return std::string(text.substr(start_byte, end_byte - start_byte));
}

std::string StringJoin(const std::vector<std::string>& items, std::string_view separator) {
    if (items.empty()) return {};
    std::string out = items.front();
    for (size_t i = 1; i < items.size(); ++i) {
        out.append(separator);
        out.append(items[i]);
    }
    return out;
}

std::vector<std::string> StringSplit(std::string_view text, const SplitOptions& opts) {
    std::vector<std::string> parts;
    if (opts.separator.empty()) {
        parts.emplace_back(text);
        return parts;
    }
    const std::string subject(text);
    if (opts.regex_mode) {
        try {
            std::regex re(opts.separator);
            std::sregex_token_iterator it(subject.begin(), subject.end(), re, -1);
            std::sregex_token_iterator end;
            for (; it != end; ++it) {
                std::string token = *it;
                if (!(opts.filter_empty && token.empty())) parts.push_back(std::move(token));
            }
            return parts;
        } catch (const std::regex_error& err) {
            throw std::runtime_error(
                std::string("StringSplit: invalid regex: ") + err.what());
        }
    }
    size_t start = 0;
    while (true) {
        const auto pos = subject.find(opts.separator, start);
        if (pos == std::string::npos) {
            std::string token = subject.substr(start);
            if (!(opts.filter_empty && token.empty())) parts.push_back(std::move(token));
            return parts;
        }
        std::string token = subject.substr(start, pos - start);
        if (!(opts.filter_empty && token.empty())) parts.push_back(std::move(token));
        start = pos + opts.separator.size();
    }
}

namespace {

const std::regex& BuiltinExtractPattern(ExtractOptions::Pattern pattern) {
    static const std::regex kNumber(R"(-?\d+(?:\.\d+)?)");
    static const std::regex kPhoneCN(R"(1[3-9]\d{9})");
    static const std::regex kEmail(R"([A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Za-z]{2,})");
    static const std::regex kIdCard(R"(\d{17}[\dXx])");
    switch (pattern) {
        case ExtractOptions::Pattern::Number:    return kNumber;
        case ExtractOptions::Pattern::PhoneCN:   return kPhoneCN;
        case ExtractOptions::Pattern::Email:     return kEmail;
        case ExtractOptions::Pattern::IdCardCN:  return kIdCard;
        case ExtractOptions::Pattern::CustomRegex:
            break;
    }
    static const std::regex kEmpty;
    return kEmpty;
}

std::regex MakeExtractRegex(const ExtractOptions& opts) {
    auto flags = std::regex_constants::ECMAScript;
    if (opts.ignore_case) flags |= std::regex_constants::icase;

    if (opts.pattern == ExtractOptions::Pattern::CustomRegex) {
        try {
            return std::regex(opts.custom_regex, flags);
        } catch (const std::regex_error& err) {
            throw std::runtime_error(
                std::string("StringExtract: invalid custom_regex: ") + err.what());
        }
    }
    if (!opts.ignore_case) {
        return BuiltinExtractPattern(opts.pattern);  // copy of cached pattern
    }
    // ignore_case=true with a built-in pattern requires a fresh compile.
    switch (opts.pattern) {
        case ExtractOptions::Pattern::Number:
            return std::regex(R"(-?\d+(?:\.\d+)?)", flags);
        case ExtractOptions::Pattern::PhoneCN:
            return std::regex(R"(1[3-9]\d{9})", flags);
        case ExtractOptions::Pattern::Email:
            return std::regex(R"([A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Za-z]{2,})", flags);
        case ExtractOptions::Pattern::IdCardCN:
            return std::regex(R"(\d{17}[\dXx])", flags);
        case ExtractOptions::Pattern::CustomRegex:
            break;
    }
    return std::regex{};
}

}  // namespace

std::vector<std::string> StringExtract(std::string_view text, const ExtractOptions& opts) {
    const auto re = MakeExtractRegex(opts);
    std::vector<std::string> matches;
    const std::string subject(text);
    auto it = std::sregex_iterator(subject.begin(), subject.end(), re);
    const auto end = std::sregex_iterator();
    for (; it != end; ++it) {
        matches.push_back(it->str());
        if (opts.only_first) break;
    }
    return matches;
}

namespace {

std::string LiteralReplace(std::string_view text,
                           std::string_view find,
                           std::string_view replace_with,
                           bool only_first,
                           bool ignore_case) {
    if (find.empty()) return std::string(text);

    auto matches_at = [&](size_t pos) {
        if (pos + find.size() > text.size()) return false;
        if (ignore_case) {
            for (size_t k = 0; k < find.size(); ++k) {
                const auto a = static_cast<unsigned char>(text[pos + k]);
                const auto b = static_cast<unsigned char>(find[k]);
                if (ToAsciiLower(a) != ToAsciiLower(b)) return false;
            }
            return true;
        }
        return std::memcmp(text.data() + pos, find.data(), find.size()) == 0;
    };

    std::string out;
    out.reserve(text.size());
    size_t pos = 0;
    while (pos < text.size()) {
        if (matches_at(pos)) {
            out.append(replace_with);
            pos += find.size();
            if (only_first) {
                out.append(text.substr(pos));
                return out;
            }
        } else {
            out.push_back(text[pos]);
            ++pos;
        }
    }
    return out;
}

std::regex MakeReplaceRegex(const ReplaceOptions& opts) {
    auto flags = std::regex_constants::ECMAScript;
    if (opts.ignore_case) flags |= std::regex_constants::icase;
    switch (opts.pattern) {
        case ReplaceOptions::Pattern::Number:
            return std::regex(R"(-?\d+(?:\.\d+)?)", flags);
        case ReplaceOptions::Pattern::PhoneCN:
            return std::regex(R"(1[3-9]\d{9})", flags);
        case ReplaceOptions::Pattern::Email:
            return std::regex(R"([A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Za-z]{2,})", flags);
        case ReplaceOptions::Pattern::IdCardCN:
            return std::regex(R"(\d{17}[\dXx])", flags);
        case ReplaceOptions::Pattern::CustomRegex:
            try {
                return std::regex(opts.find, flags);
            } catch (const std::regex_error& err) {
                throw std::runtime_error(
                    std::string("StringReplace: invalid custom_regex: ") + err.what());
            }
        case ReplaceOptions::Pattern::Literal:
            break;
    }
    return std::regex{};
}

}  // namespace

std::string StringReplace(std::string_view text, const ReplaceOptions& opts) {
    if (opts.pattern == ReplaceOptions::Pattern::Literal) {
        return LiteralReplace(text, opts.find, opts.replace_with, opts.only_first, opts.ignore_case);
    }
    const auto re = MakeReplaceRegex(opts);
    const std::string subject(text);
    if (opts.only_first) {
        return std::regex_replace(subject, re, opts.replace_with,
                                  std::regex_constants::format_first_only);
    }
    return std::regex_replace(subject, re, opts.replace_with);
}

}  // namespace mpa::data

