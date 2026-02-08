#include "csv_importer.h"
#include <iostream>
#include <cstring>
#include <functional>
#include <algorithm>
#include <chrono>

namespace gtaf::core::utility {

std::vector<std::string> split_csv_line(const std::string& line, char delim) {
    std::vector<std::string> out;
    out.reserve(16);
    std::string field;
    field.reserve(64);
    bool in_quotes = false;
    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (c == '"') {
            if (in_quotes && i + 1 < line.size() && line[i + 1] == '"') {
                // escaped quote
                field.push_back('"');
                ++i;
            } else {
                in_quotes = !in_quotes;
            }
            continue;
        }

        if (!in_quotes && c == delim) {
            out.push_back(field);
            field.clear();
            continue;
        }

        field.push_back(c);
    }
    out.push_back(field);
    return out;
}

// Simple helper to create an EntityId from table name hash + key
static types::EntityId make_entity_id(const std::string& table, uint64_t key) {
    types::EntityId e{};
    // Hash table name to 8 bytes
    std::hash<std::string> h;
    uint64_t t = static_cast<uint64_t>(h(table));
    std::memcpy(e.bytes.data(), &t, 8);
    std::memcpy(e.bytes.data() + 8, &key, 8);
    return e;
}

size_t import_csv_to_store(core::AtomStore& store,
                           const std::string& input_path,
                           const std::string& output_path,
                           const CsvImportOptions& options) {
    std::ifstream file(input_path);
    if (!file.is_open()) {
        std::cerr << "Error: cannot open " << input_path << "\n";
        return 0;
    }

    std::string line;
    if (!std::getline(file, line)) {
        std::cerr << "Error: empty file " << input_path << "\n";
        return 0;
    }

    // header
    auto headers = split_csv_line(line, options.delimiter);

    std::vector<core::AtomStore::BatchAtom> batch;
    batch.reserve(options.batch_size * std::max<size_t>(1, headers.size()));

    size_t row_count = 0;
    uint64_t row_key = 1;

    auto start = std::chrono::high_resolution_clock::now();

    while (std::getline(file, line)) {
        if (line.empty()) continue;

        auto fields = split_csv_line(line, options.delimiter);
        // create key: prefer provided key column
        uint64_t key_value = row_key;
        if (options.key_column >= 0 && static_cast<size_t>(options.key_column) < fields.size()) {
            try {
                key_value = static_cast<uint64_t>(std::stoll(fields[options.key_column]));
            } catch (...) {
                key_value = row_key;
            }
        }

        types::EntityId entity = make_entity_id(options.table_name.empty() ? "table" : options.table_name, key_value);

        for (size_t c = 0; c < headers.size() && c < fields.size(); ++c) {
            core::AtomStore::BatchAtom a;
            a.entity = entity;
            a.tag = (options.table_name.empty() ? std::string("col") : options.table_name) + "." + headers[c];
            a.value = fields[c];
            a.classification = types::AtomType::Canonical;
            batch.push_back(std::move(a));
        }

        ++row_count;
        ++row_key;

        if (batch.size() >= options.batch_size) {
            store.append_batch(batch);
            batch.clear();
        }
    }

    if (!batch.empty()) {
        store.append_batch(batch);
        batch.clear();
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "Imported " << row_count << " rows in " << ms << " ms\n";

    // Save to output path
    if (!output_path.empty()) {
        std::cout << "Saving to: " << output_path << "\n";
        if (!store.save(output_path)) {
            std::cerr << "Error saving store to " << output_path << "\n";
        }
    }

    return row_count;
}

} // namespace gtaf::core::utility
