#pragma once

#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace mpa {

class RuntimeValue {
public:
    enum class Kind {
        kString,
        kList,
        kDictionary,
        kDataTable
    };

    using List = std::vector<std::string>;
    using Dictionary = std::map<std::string, std::string>;
    struct DataTable {
        std::vector<std::string> columns;
        std::vector<std::vector<std::string>> rows;
    };

    RuntimeValue() = default;
    RuntimeValue(std::string value) : kind_(Kind::kString), string_(std::move(value)) {}

    static RuntimeValue ListValue() {
        RuntimeValue value;
        value.kind_ = Kind::kList;
        return value;
    }

    static RuntimeValue DictionaryValue() {
        RuntimeValue value;
        value.kind_ = Kind::kDictionary;
        return value;
    }

    static RuntimeValue DataTableValue() {
        RuntimeValue value;
        value.kind_ = Kind::kDataTable;
        return value;
    }

    [[nodiscard]] Kind GetKind() const { return kind_; }
    [[nodiscard]] bool IsString() const { return kind_ == Kind::kString; }
    [[nodiscard]] bool IsList() const { return kind_ == Kind::kList; }
    [[nodiscard]] bool IsDictionary() const { return kind_ == Kind::kDictionary; }
    [[nodiscard]] bool IsDataTable() const { return kind_ == Kind::kDataTable; }

    [[nodiscard]] const std::string& AsString() const { return string_; }
    [[nodiscard]] const List& AsList() const { return list_; }
    [[nodiscard]] const Dictionary& AsDictionary() const { return dictionary_; }
    [[nodiscard]] const DataTable& AsDataTable() const { return data_table_; }

    List& AsMutableList() { return list_; }
    Dictionary& AsMutableDictionary() { return dictionary_; }
    DataTable& AsMutableDataTable() { return data_table_; }

private:
    Kind kind_ = Kind::kString;
    std::string string_;
    List list_;
    Dictionary dictionary_;
    DataTable data_table_;
};

struct ExecutionContext {
    std::map<std::string, RuntimeValue> values;

    [[nodiscard]] bool Contains(const std::string& key) const {
        return values.contains(key);
    }

    [[nodiscard]] std::string GetString(const std::string& key) const {
        const auto it = values.find(key);
        if (it == values.end() || !it->second.IsString()) {
            return {};
        }
        return it->second.AsString();
    }

    [[nodiscard]] std::string RequireString(const std::string& key) const {
        const auto it = values.find(key);
        if (it == values.end()) {
            throw std::runtime_error("context value not found: " + key);
        }
        if (!it->second.IsString()) {
            throw std::runtime_error("context value is not a string: " + key);
        }
        return it->second.AsString();
    }

    void SetString(const std::string& key, std::string value) {
        values[key] = RuntimeValue(std::move(value));
    }

    void SetList(const std::string& key, RuntimeValue::List value) {
        auto item = RuntimeValue::ListValue();
        item.AsMutableList() = std::move(value);
        values[key] = std::move(item);
    }

    void SetDictionary(const std::string& key, RuntimeValue::Dictionary value) {
        auto item = RuntimeValue::DictionaryValue();
        item.AsMutableDictionary() = std::move(value);
        values[key] = std::move(item);
    }

    void SetDataTable(const std::string& key, RuntimeValue::DataTable value) {
        auto item = RuntimeValue::DataTableValue();
        item.AsMutableDataTable() = std::move(value);
        values[key] = std::move(item);
    }

    void CreateList(const std::string& key) { values[key] = RuntimeValue::ListValue(); }

    void AppendListItem(const std::string& key, std::string value) {
        auto& item = values[key];
        if (!item.IsList()) {
            throw std::runtime_error("context value is not a list: " + key);
        }
        item.AsMutableList().push_back(std::move(value));
    }

    [[nodiscard]] std::string GetListItem(const std::string& key, size_t index) const {
        const auto it = values.find(key);
        if (it == values.end() || !it->second.IsList()) {
            throw std::runtime_error("context value is not a list: " + key);
        }
        const auto& list = it->second.AsList();
        if (index >= list.size()) {
            throw std::runtime_error("list index out of range: " + key);
        }
        return list[index];
    }

    void CreateDictionary(const std::string& key) { values[key] = RuntimeValue::DictionaryValue(); }

    void CreateDataTable(const std::string& key) { values[key] = RuntimeValue::DataTableValue(); }

    void SetDictionaryValue(const std::string& key, const std::string& entry, std::string value) {
        auto& item = values[key];
        if (!item.IsDictionary()) {
            throw std::runtime_error("context value is not a dictionary: " + key);
        }
        item.AsMutableDictionary()[entry] = std::move(value);
    }

    [[nodiscard]] std::string GetDictionaryValue(
        const std::string& key,
        const std::string& entry) const {
        const auto it = values.find(key);
        if (it == values.end() || !it->second.IsDictionary()) {
            throw std::runtime_error("context value is not a dictionary: " + key);
        }
        const auto& dictionary = it->second.AsDictionary();
        const auto entry_it = dictionary.find(entry);
        if (entry_it == dictionary.end()) {
            throw std::runtime_error("dictionary entry not found: " + key + "." + entry);
        }
        return entry_it->second;
    }

    void Erase(const std::string& key) { values.erase(key); }
};

}  // namespace mpa
