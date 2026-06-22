#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <string_view>

namespace mpa::data {

struct DateTimeValue {
    int year = 1970;
    int month = 1;
    int day = 1;
    int hour = 0;
    int minute = 0;
    int second = 0;
};

enum class DateTimeUnit { Second, Minute, Hour, Day, Week, Month, Year };
enum class TimestampUnit { Second, Millisecond };

DateTimeValue DateTimeParse(std::string_view text, std::string_view format);
std::string DateTimeFormat(const DateTimeValue& value, std::string_view format);
DateTimeValue DateTimeAdd(const DateTimeValue& value, long long amount, DateTimeUnit unit);
long long DateTimeDiff(const DateTimeValue& start, const DateTimeValue& end, DateTimeUnit unit);
long long DateTimeToTimestamp(const DateTimeValue& value, TimestampUnit unit);
DateTimeValue DateTimeFromTimestamp(long long timestamp, TimestampUnit unit);
DateTimeValue DateTimeNow();
std::map<std::string, std::string> DateTimeDetails(const DateTimeValue& value);

}  // namespace mpa::data
