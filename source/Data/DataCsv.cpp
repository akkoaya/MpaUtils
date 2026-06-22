#include "MpaUtils/Data/DataCsv.h"

#include <stdexcept>

#include "MpaUtils/Core/JsonValue.h"

namespace mpa::data {

CsvRows ParseCsv(std::string_view text) {
    if (text.size() >= 3 &&
        static_cast<unsigned char>(text[0]) == 0xEF &&
        static_cast<unsigned char>(text[1]) == 0xBB &&
        static_cast<unsigned char>(text[2]) == 0xBF) {
        text.remove_prefix(3);
    }

    CsvRows rows;
    std::vector<std::string> row;
    std::string field;
    bool in_quotes = false;
    bool field_was_quoted = false;

    auto finish_field = [&]() {
        row.push_back(field);
        field.clear();
        field_was_quoted = false;
    };
    auto finish_row = [&]() {
        finish_field();
        rows.push_back(row);
        row.clear();
    };

    for (size_t i = 0; i < text.size(); ++i) {
        const char ch = text[i];
        if (in_quotes) {
            if (ch == '"') {
                if (i + 1 < text.size() && text[i + 1] == '"') {
                    field.push_back('"');
                    ++i;
                } else {
                    in_quotes = false;
                }
                continue;
            }
            field.push_back(ch);
            continue;
        }

        if (ch == '"' && field.empty() && !field_was_quoted) {
            in_quotes = true;
            field_was_quoted = true;
            continue;
        }
        if (ch == ',') {
            finish_field();
            continue;
        }
        if (ch == '\r' || ch == '\n') {
            if (ch == '\r' && i + 1 < text.size() && text[i + 1] == '\n') {
                ++i;
            }
            finish_row();
            continue;
        }
        field.push_back(ch);
    }

    if (in_quotes) {
        throw std::runtime_error("ParseCsv: unterminated quoted field");
    }
    if (!field.empty() || !row.empty() || field_was_quoted) {
        finish_row();
    }

    return rows;
}

namespace {

bool NeedsQuoting(std::string_view field) {
    return field.find_first_of(",\"\r\n") != std::string_view::npos;
}

void AppendCsvField(std::string& out, std::string_view field) {
    if (!NeedsQuoting(field)) {
        out.append(field);
        return;
    }

    out.push_back('"');
    for (const auto ch : field) {
        if (ch == '"') {
            out.append("\"\"");
        } else {
            out.push_back(ch);
        }
    }
    out.push_back('"');
}

}  // namespace

std::string SerializeCsv(const CsvRows& rows) {
    std::string out;
    for (const auto& row : rows) {
        for (size_t i = 0; i < row.size(); ++i) {
            if (i > 0) {
                out.push_back(',');
            }
            AppendCsvField(out, row[i]);
        }
        out.append("\r\n");
    }
    return out;
}

std::string CsvRowsToJson(const CsvRows& rows) {
    std::string out = "[";
    for (size_t row_index = 0; row_index < rows.size(); ++row_index) {
        if (row_index > 0) {
            out.push_back(',');
        }
        out.push_back('[');
        for (size_t column_index = 0; column_index < rows[row_index].size(); ++column_index) {
            if (column_index > 0) {
                out.push_back(',');
            }
            out.push_back('"');
            out.append(JsonEscape(rows[row_index][column_index]));
            out.push_back('"');
        }
        out.push_back(']');
    }
    out.push_back(']');
    return out;
}

CsvRows CsvRowsFromJson(std::string_view json_text) {
    const auto root = JsonParser(json_text).Parse();
    if (!root.IsArray()) {
        throw std::runtime_error("CsvRowsFromJson: root must be an array");
    }

    CsvRows rows;
    for (const auto& row_value : root.AsArray()) {
        if (!row_value.IsArray()) {
            throw std::runtime_error("CsvRowsFromJson: row must be an array");
        }

        std::vector<std::string> row;
        for (const auto& cell_value : row_value.AsArray()) {
            if (!cell_value.IsString()) {
                throw std::runtime_error("CsvRowsFromJson: cells must be strings");
            }
            row.push_back(cell_value.AsString());
        }
        rows.push_back(std::move(row));
    }
    return rows;
}

}  // namespace mpa::data

