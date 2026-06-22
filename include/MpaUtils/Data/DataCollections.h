#pragma once

#include <string>
#include <string_view>

#include "MpaUtils/Core/RuntimeValue.h"

namespace mpa::data {

enum class ListSortOrder { Ascending, Descending };
enum class ListSortMode { Text, Number };

struct ListSortOptions {
    ListSortOrder order = ListSortOrder::Ascending;
    ListSortMode mode = ListSortMode::Text;
};

RuntimeValue::List ListClear(const RuntimeValue::List& items);
RuntimeValue::List ListInsert(const RuntimeValue::List& items, long long index, std::string value);
RuntimeValue::List ListSet(const RuntimeValue::List& items, long long index, std::string value);
RuntimeValue::List ListRemoveIndex(const RuntimeValue::List& items, long long index);
RuntimeValue::List ListRemoveValue(const RuntimeValue::List& items, std::string_view value, bool all);
size_t ListLength(const RuntimeValue::List& items);
long long ListIndexOf(const RuntimeValue::List& items, std::string_view value);
RuntimeValue::List ListFilter(
    const RuntimeValue::List& items,
    const RuntimeValue::List& filter_items);
RuntimeValue::List ListSort(const RuntimeValue::List& items, ListSortOptions options);
RuntimeValue::List ListShuffle(const RuntimeValue::List& items);
RuntimeValue::List ListShuffle(const RuntimeValue::List& items, unsigned int seed);
RuntimeValue::List ListReverse(const RuntimeValue::List& items);
RuntimeValue::List ListMerge(const RuntimeValue::List& left, const RuntimeValue::List& right);
RuntimeValue::List ListDedupe(const RuntimeValue::List& items);
RuntimeValue::List ListIntersection(
    const RuntimeValue::List& left,
    const RuntimeValue::List& right);
RuntimeValue::List DictKeys(const RuntimeValue::Dictionary& dictionary);
RuntimeValue::List DictValues(const RuntimeValue::Dictionary& dictionary);
RuntimeValue::Dictionary DictRemove(
    const RuntimeValue::Dictionary& dictionary,
    std::string_view key);

}  // namespace mpa::data

