#include "MpaUtils/Data/DataDateTime.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>

namespace mpa::data {

namespace {

constexpr std::string_view kCanonicalFormat = "yyyy-MM-dd HH:mm:ss";

bool IsLeapYear(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

int DaysInMonth(int year, int month) {
    static constexpr std::array<int, 12> kDays = {
        31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month < 1 || month > 12) {
        return 0;
    }
    if (month == 2 && IsLeapYear(year)) {
        return 29;
    }
    return kDays[static_cast<size_t>(month - 1)];
}

std::tm ToTm(const DateTimeValue& value) {
    std::tm tm{};
    tm.tm_year = value.year - 1900;
    tm.tm_mon = value.month - 1;
    tm.tm_mday = value.day;
    tm.tm_hour = value.hour;
    tm.tm_min = value.minute;
    tm.tm_sec = value.second;
    tm.tm_isdst = -1;
    return tm;
}

DateTimeValue FromTm(const std::tm& tm) {
    return DateTimeValue{
        tm.tm_year + 1900,
        tm.tm_mon + 1,
        tm.tm_mday,
        tm.tm_hour,
        tm.tm_min,
        tm.tm_sec};
}

std::tm LocalTimeFromTimeT(std::time_t value) {
    std::tm tm{};
#if defined(_WIN32)
    if (localtime_s(&tm, &value) != 0) {
        throw std::runtime_error("datetime: failed to convert timestamp to local time");
    }
#else
    if (localtime_r(&value, &tm) == nullptr) {
        throw std::runtime_error("datetime: failed to convert timestamp to local time");
    }
#endif
    return tm;
}

std::time_t ToTimeT(const DateTimeValue& value) {
    auto tm = ToTm(value);
    errno = 0;
    const auto timestamp = std::mktime(&tm);
    if (timestamp == static_cast<std::time_t>(-1) && errno != 0) {
        throw std::runtime_error("datetime: value is outside supported timestamp range");
    }
    const auto normalized = FromTm(tm);
    if (normalized.year != value.year || normalized.month != value.month ||
        normalized.day != value.day || normalized.hour != value.hour ||
        normalized.minute != value.minute || normalized.second != value.second) {
        throw std::runtime_error("datetime: invalid date/time value");
    }
    return timestamp;
}

DateTimeValue Normalize(const DateTimeValue& value) {
    const auto timestamp = ToTimeT(value);
    return FromTm(LocalTimeFromTimeT(timestamp));
}

bool StartsWithAt(std::string_view text, size_t offset, std::string_view token) {
    return offset + token.size() <= text.size() && text.substr(offset, token.size()) == token;
}

int ReadFixedInt(std::string_view text, size_t offset, size_t digits, std::string_view field) {
    if (offset + digits > text.size()) {
        throw std::runtime_error("datetime.parse: missing " + std::string(field));
    }
    int value = 0;
    for (size_t i = 0; i < digits; ++i) {
        const char ch = text[offset + i];
        if (ch < '0' || ch > '9') {
            throw std::runtime_error("datetime.parse: invalid " + std::string(field));
        }
        value = value * 10 + (ch - '0');
    }
    return value;
}

void AppendPadded(std::string& out, int value, int width) {
    std::ostringstream stream;
    stream << std::setw(width) << std::setfill('0') << value;
    out += stream.str();
}

bool DayTimeLess(const DateTimeValue& lhs, const DateTimeValue& rhs) {
    return std::tie(lhs.day, lhs.hour, lhs.minute, lhs.second) <
        std::tie(rhs.day, rhs.hour, rhs.minute, rhs.second);
}

long long SecondsBetween(const DateTimeValue& start, const DateTimeValue& end) {
    return static_cast<long long>(std::difftime(ToTimeT(end), ToTimeT(start)));
}

}  // namespace

DateTimeValue DateTimeParse(std::string_view text, std::string_view format) {
    DateTimeValue value{};
    bool has_year = false;
    bool has_month = false;
    bool has_day = false;
    size_t text_pos = 0;
    size_t format_pos = 0;

    while (format_pos < format.size()) {
        if (StartsWithAt(format, format_pos, "yyyy")) {
            value.year = ReadFixedInt(text, text_pos, 4, "year");
            has_year = true;
            format_pos += 4;
            text_pos += 4;
        } else if (StartsWithAt(format, format_pos, "MM")) {
            value.month = ReadFixedInt(text, text_pos, 2, "month");
            has_month = true;
            format_pos += 2;
            text_pos += 2;
        } else if (StartsWithAt(format, format_pos, "dd")) {
            value.day = ReadFixedInt(text, text_pos, 2, "day");
            has_day = true;
            format_pos += 2;
            text_pos += 2;
        } else if (StartsWithAt(format, format_pos, "HH")) {
            value.hour = ReadFixedInt(text, text_pos, 2, "hour");
            format_pos += 2;
            text_pos += 2;
        } else if (StartsWithAt(format, format_pos, "mm")) {
            value.minute = ReadFixedInt(text, text_pos, 2, "minute");
            format_pos += 2;
            text_pos += 2;
        } else if (StartsWithAt(format, format_pos, "ss")) {
            value.second = ReadFixedInt(text, text_pos, 2, "second");
            format_pos += 2;
            text_pos += 2;
        } else {
            if (text_pos >= text.size() || text[text_pos] != format[format_pos]) {
                throw std::runtime_error("datetime.parse: literal mismatch");
            }
            ++format_pos;
            ++text_pos;
        }
    }

    if (text_pos != text.size()) {
        throw std::runtime_error("datetime.parse: trailing input");
    }
    if (!has_year || !has_month || !has_day) {
        throw std::runtime_error("datetime.parse: format requires yyyy, MM, and dd");
    }
    return Normalize(value);
}

std::string DateTimeFormat(const DateTimeValue& value, std::string_view format) {
    (void)Normalize(value);
    std::string out;
    for (size_t i = 0; i < format.size();) {
        if (StartsWithAt(format, i, "yyyy")) {
            AppendPadded(out, value.year, 4);
            i += 4;
        } else if (StartsWithAt(format, i, "MM")) {
            AppendPadded(out, value.month, 2);
            i += 2;
        } else if (StartsWithAt(format, i, "dd")) {
            AppendPadded(out, value.day, 2);
            i += 2;
        } else if (StartsWithAt(format, i, "HH")) {
            AppendPadded(out, value.hour, 2);
            i += 2;
        } else if (StartsWithAt(format, i, "mm")) {
            AppendPadded(out, value.minute, 2);
            i += 2;
        } else if (StartsWithAt(format, i, "ss")) {
            AppendPadded(out, value.second, 2);
            i += 2;
        } else {
            out.push_back(format[i]);
            ++i;
        }
    }
    return out;
}

DateTimeValue DateTimeAdd(const DateTimeValue& value, long long amount, DateTimeUnit unit) {
    if (unit == DateTimeUnit::Month || unit == DateTimeUnit::Year) {
        long long total_month = static_cast<long long>(value.year) * 12 + (value.month - 1);
        total_month += (unit == DateTimeUnit::Year) ? amount * 12 : amount;
        if (total_month < 0 ||
            total_month > static_cast<long long>(std::numeric_limits<int>::max()) * 12) {
            throw std::runtime_error("datetime.add: result is outside supported range");
        }
        DateTimeValue out = value;
        out.year = static_cast<int>(total_month / 12);
        out.month = static_cast<int>(total_month % 12) + 1;
        out.day = std::min(out.day, DaysInMonth(out.year, out.month));
        return Normalize(out);
    }

    long long seconds = amount;
    switch (unit) {
        case DateTimeUnit::Second: break;
        case DateTimeUnit::Minute: seconds *= 60; break;
        case DateTimeUnit::Hour: seconds *= 60 * 60; break;
        case DateTimeUnit::Day: seconds *= 60 * 60 * 24; break;
        case DateTimeUnit::Week: seconds *= 60 * 60 * 24 * 7; break;
        case DateTimeUnit::Month:
        case DateTimeUnit::Year:
            break;
    }
    const auto timestamp = ToTimeT(value);
    return FromTm(LocalTimeFromTimeT(timestamp + static_cast<std::time_t>(seconds)));
}

long long DateTimeDiff(const DateTimeValue& start, const DateTimeValue& end, DateTimeUnit unit) {
    if (unit == DateTimeUnit::Month) {
        long long months =
            (static_cast<long long>(end.year) - start.year) * 12 + (end.month - start.month);
        if (months > 0 && DayTimeLess(end, start)) {
            --months;
        } else if (months < 0 && DayTimeLess(start, end)) {
            ++months;
        }
        return months;
    }
    if (unit == DateTimeUnit::Year) {
        return DateTimeDiff(start, end, DateTimeUnit::Month) / 12;
    }

    const auto seconds = SecondsBetween(start, end);
    switch (unit) {
        case DateTimeUnit::Second: return seconds;
        case DateTimeUnit::Minute: return seconds / 60;
        case DateTimeUnit::Hour: return seconds / (60 * 60);
        case DateTimeUnit::Day: return seconds / (60 * 60 * 24);
        case DateTimeUnit::Week: return seconds / (60 * 60 * 24 * 7);
        case DateTimeUnit::Month:
        case DateTimeUnit::Year:
            break;
    }
    return seconds;
}

long long DateTimeToTimestamp(const DateTimeValue& value, TimestampUnit unit) {
    const auto seconds = static_cast<long long>(ToTimeT(value));
    if (unit == TimestampUnit::Millisecond) {
        return seconds * 1000;
    }
    return seconds;
}

DateTimeValue DateTimeFromTimestamp(long long timestamp, TimestampUnit unit) {
    if (unit == TimestampUnit::Millisecond) {
        timestamp /= 1000;
    }
    return FromTm(LocalTimeFromTimeT(static_cast<std::time_t>(timestamp)));
}

DateTimeValue DateTimeNow() {
    const auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    return FromTm(LocalTimeFromTimeT(now));
}

std::map<std::string, std::string> DateTimeDetails(const DateTimeValue& value) {
    const auto timestamp = ToTimeT(value);
    const auto tm = LocalTimeFromTimeT(timestamp);

    std::map<std::string, std::string> details;
    details["year"] = std::to_string(tm.tm_year + 1900);
    details["month"] = std::to_string(tm.tm_mon + 1);
    details["day"] = std::to_string(tm.tm_mday);
    details["hour"] = std::to_string(tm.tm_hour);
    details["minute"] = std::to_string(tm.tm_min);
    details["second"] = std::to_string(tm.tm_sec);
    details["weekday"] = std::to_string(tm.tm_wday);
    details["day_of_year"] = std::to_string(tm.tm_yday + 1);
    details["timestamp"] = std::to_string(static_cast<long long>(timestamp));
    return details;
}

}  // namespace mpa::data

