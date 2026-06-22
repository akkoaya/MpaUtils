#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace mpa::data {

using CsvRows = std::vector<std::vector<std::string>>;

CsvRows ParseCsv(std::string_view text);
std::string SerializeCsv(const CsvRows& rows);
std::string CsvRowsToJson(const CsvRows& rows);
CsvRows CsvRowsFromJson(std::string_view json_text);

}  // namespace mpa::data
