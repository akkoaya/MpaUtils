#include "MpaUtils/Data/DataTable.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <set>
#include <sstream>
#include <stdexcept>

#include "MpaUtils/Core/JsonValue.h"

namespace mpa::data {

namespace {

std::string TrimAscii(std::string_view value) {
    size_t start = 0;
    while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start])) != 0) {
        ++start;
    }
    size_t end = value.size();
    while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1])) != 0) {
        --end;
    }
    return std::string(value.substr(start, end - start));
}

std::vector<std::string> Split(std::string_view value, char delimiter) {
    std::vector<std::string> parts;
    size_t cursor = 0;
    while (cursor <= value.size()) {
        const auto next = value.find(delimiter, cursor);
        const auto end = next == std::string_view::npos ? value.size() : next;
        parts.push_back(TrimAscii(value.substr(cursor, end - cursor)));
        if (next == std::string_view::npos) {
            break;
        }
        cursor = next + 1;
    }
    return parts;
}

bool IsIntegerText(std::string_view value) {
    if (value.empty()) {
        return false;
    }
    size_t index = 0;
    if (value[0] == '-' || value[0] == '+') {
        index = 1;
    }
    if (index == value.size()) {
        return false;
    }
    for (; index < value.size(); ++index) {
        if (std::isdigit(static_cast<unsigned char>(value[index])) == 0) {
            return false;
        }
    }
    return true;
}

long long ParseInteger(std::string_view value, const std::string& description) {
    try {
        size_t consumed = 0;
        const auto parsed = std::stoll(std::string(value), &consumed);
        if (consumed != value.size()) {
            throw std::invalid_argument("trailing text");
        }
        return parsed;
    } catch (const std::exception&) {
        throw std::runtime_error(description + " must be an integer");
    }
}

std::string NumberToString(double value) {
    const auto rounded = std::round(value);
    if (std::fabs(value - rounded) < 0.0000001) {
        return std::to_string(static_cast<long long>(rounded));
    }
    std::ostringstream stream;
    stream << value;
    return stream.str();
}

std::string ScalarJsonToString(const JsonValue& value) {
    if (value.IsString()) {
        return value.AsString();
    }
    if (value.IsNumber()) {
        return NumberToString(value.AsNumber());
    }
    if (value.IsBool()) {
        return value.AsBool() ? "true" : "false";
    }
    if (std::holds_alternative<std::nullptr_t>(value.storage)) {
        return {};
    }
    throw std::runtime_error("datatable JSON cells must be scalar values");
}

bool IsColumnLabel(std::string_view text) {
    if (text.empty()) {
        return false;
    }
    for (const auto ch : text) {
        if (std::isalpha(static_cast<unsigned char>(ch)) == 0) {
            return false;
        }
    }
    return true;
}

size_t ParseColumnLabel(std::string_view text) {
    if (!IsColumnLabel(text)) {
        throw std::runtime_error("datatable column must be A-style label or 1-based index");
    }
    size_t value = 0;
    for (const auto ch : text) {
        value = value * 26 + static_cast<size_t>(std::toupper(static_cast<unsigned char>(ch)) - 'A' + 1);
    }
    return value - 1;
}

size_t ResolveOneBasedIndex(
    std::string_view text,
    size_t size,
    bool existing,
    const std::string& description) {
    const auto trimmed = TrimAscii(text);
    if (trimmed.empty()) {
        throw std::runtime_error(description + " is required");
    }

    const auto raw = ParseInteger(trimmed, description);
    if (raw == 0) {
        throw std::runtime_error(description + " is 1-based");
    }

    long long resolved = 0;
    if (raw < 0) {
        if (size == 0) {
            throw std::runtime_error(description + " is out of range");
        }
        resolved = static_cast<long long>(size) + raw;
    } else {
        resolved = raw - 1;
    }

    if (resolved < 0 || (existing && static_cast<size_t>(resolved) >= size)) {
        throw std::runtime_error(description + " is out of range");
    }
    return static_cast<size_t>(resolved);
}

size_t ResolveColumnIndex(
    const DataTable& table,
    std::string_view text,
    bool existing,
    const std::string& description) {
    const auto trimmed = TrimAscii(text);
    if (trimmed.empty()) {
        throw std::runtime_error(description + " is required");
    }

    const auto size = DataTableColumnCount(table);
    size_t resolved = 0;
    if (IsColumnLabel(trimmed)) {
        resolved = ParseColumnLabel(trimmed);
    } else if (IsIntegerText(trimmed)) {
        resolved = ResolveOneBasedIndex(trimmed, size, existing, description);
    } else {
        throw std::runtime_error(description + " must be A-style label or 1-based index");
    }

    if (existing && resolved >= size) {
        throw std::runtime_error(description + " is out of range");
    }
    return resolved;
}

size_t ResolveWritableRowIndex(const DataTable& table, std::string_view row) {
    const auto trimmed = TrimAscii(row);
    if (!trimmed.empty() && trimmed[0] == '-') {
        return ResolveOneBasedIndex(trimmed, table.rows.size(), true, "datatable row");
    }
    return ResolveOneBasedIndex(trimmed, table.rows.size(), false, "datatable row");
}

size_t ResolveWritableColumnIndex(const DataTable& table, std::string_view column) {
    const auto trimmed = TrimAscii(column);
    if (!trimmed.empty() && trimmed[0] == '-') {
        return ResolveColumnIndex(table, trimmed, true, "datatable column");
    }
    return ResolveColumnIndex(table, trimmed, false, "datatable column");
}

size_t ResolveInsertRowIndex(const DataTable& table, std::string_view row) {
    const auto trimmed = TrimAscii(row);
    if (!trimmed.empty() && trimmed[0] == '-') {
        return ResolveOneBasedIndex(trimmed, table.rows.size(), true, "datatable row");
    }
    const auto resolved = ResolveOneBasedIndex(trimmed, table.rows.size(), false, "datatable row");
    if (resolved > table.rows.size()) {
        throw std::runtime_error("datatable row is out of range");
    }
    return resolved;
}

size_t ResolveInsertColumnIndex(const DataTable& table, std::string_view column) {
    const auto trimmed = TrimAscii(column);
    if (!trimmed.empty() && trimmed[0] == '-') {
        return ResolveColumnIndex(table, trimmed, true, "datatable column");
    }
    const auto resolved = ResolveColumnIndex(table, trimmed, false, "datatable column");
    if (resolved > DataTableColumnCount(table)) {
        throw std::runtime_error("datatable column is out of range");
    }
    return resolved;
}

void EnsureRows(DataTable& table, size_t row_count) {
    if (table.rows.size() < row_count) {
        table.rows.resize(row_count);
    }
}

void EnsureCell(DataTable& table, size_t row_index, size_t column_index) {
    EnsureRows(table, row_index + 1);
    if (table.rows[row_index].size() <= column_index) {
        table.rows[row_index].resize(column_index + 1);
    }
}

void EnsureColumn(DataTable& table, size_t column_index) {
    for (auto& row : table.rows) {
        if (row.size() <= column_index) {
            row.resize(column_index + 1);
        }
    }
}

bool RowHasContent(const std::vector<std::string>& row) {
    return std::any_of(row.begin(), row.end(), [](const std::string& cell) {
        return !cell.empty();
    });
}

std::vector<size_t> ParseIndexSpec(
    std::string_view spec,
    const DataTable& table,
    bool columns) {
    std::set<size_t> indexes;
    for (const auto& token : Split(spec, ',')) {
        if (token.empty()) {
            continue;
        }
        const auto separator = token.find(':');
        if (separator == std::string::npos) {
            indexes.insert(columns
                ? DataTableResolveExistingColumnIndex(table, token)
                : DataTableResolveExistingRowIndex(table, token));
            continue;
        }

        const auto first_text = token.substr(0, separator);
        const auto last_text = token.substr(separator + 1);
        const auto first = columns
            ? DataTableResolveExistingColumnIndex(table, first_text)
            : DataTableResolveExistingRowIndex(table, first_text);
        const auto last = columns
            ? DataTableResolveExistingColumnIndex(table, last_text)
            : DataTableResolveExistingRowIndex(table, last_text);
        const auto low = std::min(first, last);
        const auto high = std::max(first, last);
        for (size_t index = low; index <= high; ++index) {
            indexes.insert(index);
        }
    }

    std::vector<size_t> result(indexes.begin(), indexes.end());
    std::sort(result.rbegin(), result.rend());
    return result;
}

}  // namespace

DataTable MakeDataTableFromRows(const CsvRows& rows, bool has_header) {
    DataTable table;
    if (has_header && !rows.empty()) {
        table.columns = rows.front();
        table.rows.assign(rows.begin() + 1, rows.end());
        return table;
    }
    table.rows = rows;
    return table;
}

CsvRows DataTableToRows(const DataTable& table, bool include_headers) {
    const auto width = DataTableColumnCount(table);
    CsvRows rows;
    if (include_headers) {
        std::vector<std::string> header(width);
        for (size_t column = 0; column < std::min(width, table.columns.size()); ++column) {
            header[column] = table.columns[column];
        }
        rows.push_back(std::move(header));
    }
    for (const auto& source_row : table.rows) {
        std::vector<std::string> row(width);
        for (size_t column = 0; column < std::min(width, source_row.size()); ++column) {
            row[column] = source_row[column];
        }
        rows.push_back(std::move(row));
    }
    return rows;
}

size_t DataTableColumnCount(const DataTable& table) {
    size_t count = table.columns.size();
    for (const auto& row : table.rows) {
        count = std::max(count, row.size());
    }
    return count;
}

size_t DataTableLastNonEmptyRowCount(const DataTable& table) {
    for (size_t row = table.rows.size(); row > 0; --row) {
        if (RowHasContent(table.rows[row - 1])) {
            return row;
        }
    }
    return 0;
}

std::string DataTableColumnLabel(size_t column_index) {
    std::string label;
    size_t value = column_index + 1;
    while (value > 0) {
        const auto remainder = (value - 1) % 26;
        label.insert(label.begin(), static_cast<char>('A' + remainder));
        value = (value - 1) / 26;
    }
    return label;
}

size_t DataTableResolveExistingRowIndex(const DataTable& table, std::string_view row) {
    return ResolveOneBasedIndex(TrimAscii(row), table.rows.size(), true, "datatable row");
}

size_t DataTableResolveExistingColumnIndex(const DataTable& table, std::string_view column) {
    return ResolveColumnIndex(table, column, true, "datatable column");
}

RuntimeValue::List ParseStringArrayJson(std::string_view json_text) {
    const auto root = JsonParser(json_text).Parse();
    if (!root.IsArray()) {
        throw std::runtime_error("datatable values must be a JSON array");
    }

    RuntimeValue::List values;
    values.reserve(root.AsArray().size());
    for (const auto& item : root.AsArray()) {
        values.push_back(ScalarJsonToString(item));
    }
    return values;
}

CsvRows ParseStringRowsJson(std::string_view json_text) {
    const auto root = JsonParser(json_text).Parse();
    if (!root.IsArray()) {
        throw std::runtime_error("datatable rows must be a 2d JSON array");
    }

    CsvRows rows;
    rows.reserve(root.AsArray().size());
    for (const auto& row_value : root.AsArray()) {
        if (!row_value.IsArray()) {
            throw std::runtime_error("datatable rows must be a 2d JSON array");
        }
        RuntimeValue::List row;
        row.reserve(row_value.AsArray().size());
        for (const auto& cell : row_value.AsArray()) {
            row.push_back(ScalarJsonToString(cell));
        }
        rows.push_back(std::move(row));
    }
    return rows;
}

std::string DataTableReadCell(
    const DataTable& table,
    std::string_view row,
    std::string_view column) {
    const auto row_index = DataTableResolveExistingRowIndex(table, row);
    const auto column_index = DataTableResolveExistingColumnIndex(table, column);
    const auto& source_row = table.rows[row_index];
    return column_index < source_row.size() ? source_row[column_index] : std::string{};
}

RuntimeValue::List DataTableReadRowAt(const DataTable& table, size_t row_index) {
    if (row_index >= table.rows.size()) {
        throw std::runtime_error("datatable row is out of range");
    }
    const auto width = DataTableColumnCount(table);
    RuntimeValue::List row(width);
    const auto& source_row = table.rows[row_index];
    for (size_t column = 0; column < std::min(width, source_row.size()); ++column) {
        row[column] = source_row[column];
    }
    return row;
}

RuntimeValue::List DataTableReadRow(const DataTable& table, std::string_view row) {
    return DataTableReadRowAt(table, DataTableResolveExistingRowIndex(table, row));
}

RuntimeValue::List DataTableReadColumnAt(const DataTable& table, size_t column_index) {
    if (column_index >= DataTableColumnCount(table)) {
        throw std::runtime_error("datatable column is out of range");
    }
    RuntimeValue::List column;
    column.reserve(table.rows.size());
    for (const auto& row : table.rows) {
        column.push_back(column_index < row.size() ? row[column_index] : std::string{});
    }
    return column;
}

RuntimeValue::List DataTableReadColumn(const DataTable& table, std::string_view column) {
    return DataTableReadColumnAt(table, DataTableResolveExistingColumnIndex(table, column));
}

CsvRows DataTableReadRange(
    const DataTable& table,
    std::string_view start_row,
    std::string_view start_column,
    std::string_view end_row,
    std::string_view end_column) {
    const auto row_start = DataTableResolveExistingRowIndex(table, start_row);
    const auto row_end = DataTableResolveExistingRowIndex(table, end_row);
    const auto column_start = DataTableResolveExistingColumnIndex(table, start_column);
    const auto column_end = DataTableResolveExistingColumnIndex(table, end_column);
    if (row_start > row_end || column_start > column_end) {
        throw std::runtime_error("datatable range start must be before range end");
    }

    CsvRows rows;
    rows.reserve(row_end - row_start + 1);
    for (size_t row = row_start; row <= row_end; ++row) {
        std::vector<std::string> values;
        values.reserve(column_end - column_start + 1);
        for (size_t column = column_start; column <= column_end; ++column) {
            const auto& source_row = table.rows[row];
            values.push_back(column < source_row.size() ? source_row[column] : std::string{});
        }
        rows.push_back(std::move(values));
    }
    return rows;
}

void DataTableWriteCell(
    DataTable& table,
    std::string_view row,
    std::string_view column,
    std::string value) {
    const auto row_index = ResolveWritableRowIndex(table, row);
    const auto column_index = ResolveWritableColumnIndex(table, column);
    EnsureCell(table, row_index, column_index);
    table.rows[row_index][column_index] = std::move(value);
}

void DataTableWriteRow(
    DataTable& table,
    std::string_view row,
    const RuntimeValue::List& values,
    DataTableWriteMode mode) {
    size_t row_index = 0;
    if (mode == DataTableWriteMode::kAppend) {
        row_index = DataTableLastNonEmptyRowCount(table);
        EnsureRows(table, row_index + 1);
    } else if (mode == DataTableWriteMode::kInsert) {
        row_index = ResolveInsertRowIndex(table, row);
        table.rows.insert(table.rows.begin() + static_cast<std::ptrdiff_t>(row_index), {});
    } else {
        row_index = ResolveWritableRowIndex(table, row);
        EnsureRows(table, row_index + 1);
    }

    const auto width = std::max(DataTableColumnCount(table), values.size());
    table.rows[row_index] = values;
    table.rows[row_index].resize(width);
}

void DataTableWriteColumn(
    DataTable& table,
    std::string_view column,
    const RuntimeValue::List& values,
    DataTableWriteMode mode) {
    size_t column_index = 0;
    if (mode == DataTableWriteMode::kAppend) {
        column_index = DataTableColumnCount(table);
    } else if (mode == DataTableWriteMode::kInsert) {
        column_index = ResolveInsertColumnIndex(table, column);
        if (column_index <= table.columns.size()) {
            table.columns.insert(
                table.columns.begin() + static_cast<std::ptrdiff_t>(column_index),
                {});
        }
        for (auto& row : table.rows) {
            if (column_index <= row.size()) {
                row.insert(row.begin() + static_cast<std::ptrdiff_t>(column_index), {});
            }
        }
    } else {
        column_index = ResolveWritableColumnIndex(table, column);
    }

    EnsureRows(table, values.size());
    EnsureColumn(table, column_index);
    for (size_t row = 0; row < table.rows.size(); ++row) {
        table.rows[row][column_index] = row < values.size() ? values[row] : std::string{};
    }
}

void DataTableWriteRange(
    DataTable& table,
    std::string_view start_row,
    std::string_view start_column,
    const CsvRows& values) {
    const auto row_start = ResolveWritableRowIndex(table, start_row);
    const auto column_start = ResolveWritableColumnIndex(table, start_column);
    for (size_t row = 0; row < values.size(); ++row) {
        for (size_t column = 0; column < values[row].size(); ++column) {
            EnsureCell(table, row_start + row, column_start + column);
            table.rows[row_start + row][column_start + column] = values[row][column];
        }
    }
}

void DataTableDeleteRows(DataTable& table, std::string_view rows) {
    for (const auto index : ParseIndexSpec(rows, table, false)) {
        table.rows.erase(table.rows.begin() + static_cast<std::ptrdiff_t>(index));
    }
}

void DataTableDeleteColumns(DataTable& table, std::string_view columns) {
    for (const auto index : ParseIndexSpec(columns, table, true)) {
        if (index < table.columns.size()) {
            table.columns.erase(table.columns.begin() + static_cast<std::ptrdiff_t>(index));
        }
        for (auto& row : table.rows) {
            if (index < row.size()) {
                row.erase(row.begin() + static_cast<std::ptrdiff_t>(index));
            }
        }
    }
}

void DataTableClear(DataTable& table, bool preserve_columns) {
    table.rows.clear();
    if (!preserve_columns) {
        table.columns.clear();
    }
}

std::string DataTableGetColumnDescription(const DataTable& table, std::string_view column) {
    const auto index = DataTableResolveExistingColumnIndex(table, column);
    return index < table.columns.size() ? table.columns[index] : std::string{};
}

std::string DataTableFindColumnByDescription(
    const DataTable& table,
    std::string_view description) {
    for (size_t index = 0; index < table.columns.size(); ++index) {
        if (table.columns[index] == description) {
            return DataTableColumnLabel(index);
        }
    }
    return {};
}

void DataTableSetColumnDescription(
    DataTable& table,
    std::string_view column,
    std::string description) {
    const auto index = ResolveWritableColumnIndex(table, column);
    if (table.columns.size() <= index) {
        table.columns.resize(index + 1);
    }
    table.columns[index] = std::move(description);
}

}  // namespace mpa::data

