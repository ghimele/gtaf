#pragma once

#include "../../core/utility/csv_importer.h"

namespace gtaf::cli::importer {

using CsvImportOptions = gtaf::core::utility::CsvImportOptions;

inline std::vector<std::string> split_csv_line(const std::string& line, char delim) {
    return gtaf::core::utility::split_csv_line(line, delim);
}

inline size_t import_csv_to_store(core::AtomStore& store,
                                  const std::string& input_path,
                                  const std::string& output_path,
                                  const CsvImportOptions& options) {
    return gtaf::core::utility::import_csv_to_store(store, input_path, output_path, options);
}

} // namespace gtaf::cli::importer
