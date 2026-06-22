#include "MpaUtils/Data/DataCollections.h"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <random>
#include <set>
#include <stdexcept>

namespace mpa::data {

namespace {

size_t NormalizeListIndex(long long index, size_t size, bool allow_end, std::string_view action) {
    const long long signed_size = static_cast<long long>(size);
    long long normalized = index;
    if (normalized < 0) {
        normalized += signed_size;
    }

    const long long max_index = allow_end ? signed_size : signed_size - 1;
    if (normalized < 0 || normalized > max_index || (!allow_end && size == 0)) {
        throw std::runtime_error(std::string(action) + ": list index out of range");
    }
    return static_cast<size_t>(normalized);
}

double ParseNumber(std::string_view text) {
    double value = 0.0;
    const char* begin = text.data();
    const char* end = text.data() + text.size();
    const auto [ptr, ec] = std::from_chars(begin, end, value);
    if (ec != std::errc() || ptr != end || !std::isfinite(value)) {
        throw std::runtime_error("data.list.sort: item is not a number: " + std::string(text));
    }
    return value;
}

}  // namespace

RuntimeValue::List ListClear(const RuntimeValue::List& items) {
    (void)items;
    return {};
}

RuntimeValue::List ListInsert(const RuntimeValue::List& items, long long index, std::string value) {
    auto out = items;
    const auto pos = NormalizeListIndex(index, out.size(), true, "data.list.insert");
    out.insert(out.begin() + static_cast<RuntimeValue::List::difference_type>(pos), std::move(value));
    return out;
}

RuntimeValue::List ListSet(const RuntimeValue::List& items, long long index, std::string value) {
    auto out = items;
    const auto pos = NormalizeListIndex(index, out.size(), false, "data.list.set");
    out[pos] = std::move(value);
    return out;
}

RuntimeValue::List ListRemoveIndex(const RuntimeValue::List& items, long long index) {
    auto out = items;
    const auto pos = NormalizeListIndex(index, out.size(), false, "data.list.remove");
    out.erase(out.begin() + static_cast<RuntimeValue::List::difference_type>(pos));
    return out;
}

RuntimeValue::List ListRemoveValue(const RuntimeValue::List& items, std::string_view value, bool all) {
    RuntimeValue::List out;
    out.reserve(items.size());
    bool removed = false;
    for (const auto& item : items) {
        if (item == value && (all || !removed)) {
            removed = true;
            continue;
        }
        out.push_back(item);
    }
    return out;
}

size_t ListLength(const RuntimeValue::List& items) {
    return items.size();
}

long long ListIndexOf(const RuntimeValue::List& items, std::string_view value) {
    for (size_t index = 0; index < items.size(); ++index) {
        if (items[index] == value) {
            return static_cast<long long>(index);
        }
    }
    return -1;
}

RuntimeValue::List ListFilter(
    const RuntimeValue::List& items,
    const RuntimeValue::List& filter_items) {
    const std::set<std::string> excluded(filter_items.begin(), filter_items.end());
    RuntimeValue::List out;
    out.reserve(items.size());
    for (const auto& item : items) {
        if (!excluded.contains(item)) {
            out.push_back(item);
        }
    }
    return out;
}

RuntimeValue::List ListSort(const RuntimeValue::List& items, ListSortOptions options) {
    auto out = items;
    if (options.mode == ListSortMode::Text) {
        std::sort(out.begin(), out.end());
    } else {
        std::sort(
            out.begin(),
            out.end(),
            [](const std::string& lhs, const std::string& rhs) {
                return ParseNumber(lhs) < ParseNumber(rhs);
            });
    }
    if (options.order == ListSortOrder::Descending) {
        std::reverse(out.begin(), out.end());
    }
    return out;
}

RuntimeValue::List ListShuffle(const RuntimeValue::List& items) {
    std::random_device device;
    return ListShuffle(items, device());
}

RuntimeValue::List ListShuffle(const RuntimeValue::List& items, unsigned int seed) {
    auto out = items;
    std::mt19937 generator(seed);
    std::shuffle(out.begin(), out.end(), generator);
    return out;
}

RuntimeValue::List ListReverse(const RuntimeValue::List& items) {
    auto out = items;
    std::reverse(out.begin(), out.end());
    return out;
}

RuntimeValue::List ListMerge(const RuntimeValue::List& left, const RuntimeValue::List& right) {
    auto out = left;
    out.reserve(left.size() + right.size());
    out.insert(out.end(), right.begin(), right.end());
    return out;
}

RuntimeValue::List ListDedupe(const RuntimeValue::List& items) {
    RuntimeValue::List out;
    out.reserve(items.size());
    std::set<std::string> seen;
    for (const auto& item : items) {
        if (seen.insert(item).second) {
            out.push_back(item);
        }
    }
    return out;
}

RuntimeValue::List ListIntersection(
    const RuntimeValue::List& left,
    const RuntimeValue::List& right) {
    const std::set<std::string> right_items(right.begin(), right.end());
    std::set<std::string> emitted;
    RuntimeValue::List out;
    out.reserve(std::min(left.size(), right.size()));
    for (const auto& item : left) {
        if (right_items.contains(item) && emitted.insert(item).second) {
            out.push_back(item);
        }
    }
    return out;
}

RuntimeValue::List DictKeys(const RuntimeValue::Dictionary& dictionary) {
    RuntimeValue::List out;
    out.reserve(dictionary.size());
    for (const auto& [key, value] : dictionary) {
        (void)value;
        out.push_back(key);
    }
    return out;
}

RuntimeValue::List DictValues(const RuntimeValue::Dictionary& dictionary) {
    RuntimeValue::List out;
    out.reserve(dictionary.size());
    for (const auto& [key, value] : dictionary) {
        (void)key;
        out.push_back(value);
    }
    return out;
}

RuntimeValue::Dictionary DictRemove(
    const RuntimeValue::Dictionary& dictionary,
    std::string_view key) {
    auto out = dictionary;
    out.erase(std::string(key));
    return out;
}

}  // namespace mpa::data


