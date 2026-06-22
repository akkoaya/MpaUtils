#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "MpaUtils/Data/DataCsv.h"
#include "MpaUtils/Core/RuntimeValue.h"

namespace mpa::data {

using DataTable = RuntimeValue::DataTable;

enum class DataTableWriteMode {
    kOverwrite,
    kAppend,
    kInsert
};

DataTable MakeDataTableFromRows(const CsvRows& rows, bool has_header);
CsvRows DataTableToRows(const DataTable& table, bool include_headers);

size_t DataTableColumnCount(const DataTable& table);
size_t DataTableLastNonEmptyRowCount(const DataTable& table);
std::string DataTableColumnLabel(size_t column_index);
size_t DataTableResolveExistingRowIndex(const DataTable& table, std::string_view row);
size_t DataTableResolveExistingColumnIndex(const DataTable& table, std::string_view column);

RuntimeValue::List ParseStringArrayJson(std::string_view json_text);
CsvRows ParseStringRowsJson(std::string_view json_text);

std::string DataTableReadCell(
    const DataTable& table,
    std::string_view row,
    std::string_view column);
RuntimeValue::List DataTableReadRow(const DataTable& table, std::string_view row);
RuntimeValue::List DataTableReadColumn(const DataTable& table, std::string_view column);
CsvRows DataTableReadRange(
    const DataTable& table,
    std::string_view start_row,
    std::string_view start_column,
    std::string_view end_row,
    std::string_view end_column);
RuntimeValue::List DataTableReadRowAt(const DataTable& table, size_t row_index);
RuntimeValue::List DataTableReadColumnAt(const DataTable& table, size_t column_index);

void DataTableWriteCell(
    DataTable& table,
    std::string_view row,
    std::string_view column,
    std::string value);
void DataTableWriteRow(
    DataTable& table,
    std::string_view row,
    const RuntimeValue::List& values,
    DataTableWriteMode mode);
void DataTableWriteColumn(
    DataTable& table,
    std::string_view column,
    const RuntimeValue::List& values,
    DataTableWriteMode mode);
void DataTableWriteRange(
    DataTable& table,
    std::string_view start_row,
    std::string_view start_column,
    const CsvRows& values);

void DataTableDeleteRows(DataTable& table, std::string_view rows);
void DataTableDeleteColumns(DataTable& table, std::string_view columns);
void DataTableClear(DataTable& table, bool preserve_columns);

std::string DataTableGetColumnDescription(const DataTable& table, std::string_view column);
std::string DataTableFindColumnByDescription(
    const DataTable& table,
    std::string_view description);
void DataTableSetColumnDescription(
    DataTable& table,
    std::string_view column,
    std::string description);

}  // namespace mpa::data

