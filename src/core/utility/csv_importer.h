#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <functional>
#include "../atom_store.h"

namespace gtaf::core::utility {

struct CsvImportOptions {
    char delimiter = ',';
    size_t batch_size = 50000;
    // If provided, use this column index as entity key (0-based). If -1, use row counter.
    int key_column = -1;
    std::string table_name;
};

// Parse CSV line into fields (very small, efficient parser)
std::vector<std::string> split_csv_line(const std::string& line, char delim);

// Import CSV into the provided AtomStore. Returns number of rows imported.
size_t import_csv_to_store(
    core::AtomStore& store,
    const std::string& input_path,
    const std::string& output_path,
    const CsvImportOptions& options
);

} // namespace gtaf::core::utility
