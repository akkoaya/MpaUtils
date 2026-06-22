#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace mpa::data {

// Counts UTF-8 codepoints. Malformed bytes are counted as one codepoint each
// so the function never throws.
size_t StringLength(std::string_view text);

enum class TrimSide { Left, Right, Both };

// Strips ASCII whitespace (space, tab, CR, LF, VT, FF) and the CJK full-width
// space U+3000 from the requested side(s).
std::string StringTrim(std::string_view text, TrimSide side);

enum class CaseMode { Upper, Lower, Title };

// ASCII-only case folding. Bytes outside 0x00-0x7F are passed through verbatim.
// Title mode uppercases an ASCII letter when it follows a non-letter (or is the
// first byte) and lowercases all other ASCII letters.
std::string StringChangeCase(std::string_view text, CaseMode mode);

// When newline=true the result is base + "\n" + suffix.
// When newline=false the result is base + suffix.
std::string StringAppend(std::string_view base, std::string_view suffix, bool newline);

enum class PadSide { Left, Right };

// Pads `text` to `total_codepoints` codepoints by repeating `fill` on `side`.
// - If text already has >= total_codepoints, returns text unchanged.
// - If fill is shorter than the gap, repeats fill to fill the gap exactly.
// - If fill is longer than the gap, only the leading codepoints that fit are appended.
// - Throws std::invalid_argument if padding is required and fill is empty.
std::string StringPad(std::string_view text,
                      std::string_view fill,
                      size_t total_codepoints,
                      PadSide side);

struct SubstringOptions {
    enum class StartMode { FromFirst, ByIndex, ByText };
    enum class LengthMode { ToEnd, ByLength };
    StartMode start_mode = StartMode::FromFirst;
    long start_index = 0;            // ByIndex: 0-based codepoint index, must be >= 0
    std::string start_text;          // ByText
    LengthMode length_mode = LengthMode::ToEnd;
    long length_value = 0;           // ByLength: codepoint count, must be >= 0
};
// start_index<0 or length_value<0 throws std::invalid_argument.
// start_text not found returns "".
// start_index past end returns "".
// ByLength running past end returns text from start to end (no padding, no throw).
std::string StringSubstring(std::string_view text, const SubstringOptions& opts);

std::string StringJoin(const std::vector<std::string>& items, std::string_view separator);

struct SplitOptions {
    std::string separator;          // literal text or regex pattern
    bool regex_mode = false;
    bool filter_empty = false;
};
// Throws std::runtime_error if regex_mode && separator fails to compile.
std::vector<std::string> StringSplit(std::string_view text, const SplitOptions& opts);

struct ExtractOptions {
    enum class Pattern { Number, PhoneCN, Email, IdCardCN, CustomRegex };
    Pattern pattern = Pattern::CustomRegex;
    std::string custom_regex;       // used only when Pattern::CustomRegex
    bool only_first = true;
    bool ignore_case = false;
};
// Returns one element when only_first=true and a match exists, otherwise all matches
// in document order. Returns empty when nothing matches.
// Throws std::runtime_error if Pattern::CustomRegex and custom_regex fails to compile.
std::vector<std::string> StringExtract(std::string_view text, const ExtractOptions& opts);

struct ReplaceOptions {
    enum class Pattern { Literal, Number, PhoneCN, Email, IdCardCN, CustomRegex };
    Pattern pattern = Pattern::Literal;
    std::string find;               // Literal / CustomRegex use this
    std::string replace_with;
    bool only_first = false;
    bool ignore_case = false;
};
// Throws std::runtime_error if Pattern::CustomRegex and find fails to compile.
// Pattern::Literal with empty find returns text unchanged.
std::string StringReplace(std::string_view text, const ReplaceOptions& opts);

}  // namespace mpa::data
