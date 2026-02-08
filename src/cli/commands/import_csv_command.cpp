#include "import_csv_command.h"
#include "../../core/atom_store.h"
#include "../../core/utility/csv_importer.h"
#include <sstream>
#include <iostream>

namespace gtaf::cli {

Result ImportCsvCommand::execute(const Command& cmd, Session& session) {
    if (cmd.positionals.size() < 2) {
        return Result::failure("Usage: " + description());
    }

    const std::string& input = cmd.positionals[0];
    const std::string& output = cmd.positionals[1];

    core::utility::CsvImportOptions opts;
    if (cmd.options.count("table")) opts.table_name = cmd.options.at("table");
    if (cmd.options.count("key-col")) {
        try { opts.key_column = std::stoi(cmd.options.at("key-col")); } catch(...) { opts.key_column = -1; }
    }
    if (cmd.options.count("batch-size")) {
        try { opts.batch_size = static_cast<size_t>(std::stoull(cmd.options.at("batch-size"))); } catch(...) {}
    }
    // delimiter option: support --delimiter or --delim
    if (cmd.options.count("delimiter")) {
        auto d = cmd.options.at("delimiter");
        if (!d.empty()) opts.delimiter = d[0];
    } else if (cmd.options.count("delim")) {
        auto d = cmd.options.at("delim");
        if (!d.empty()) opts.delimiter = d[0];
    }

    // Ensure session store exists
    auto& store = session.get_store();

    size_t imported = core::utility::import_csv_to_store(store, input, output, opts);

    std::ostringstream oss;
    oss << "Imported rows: " << imported << "\n";
    return Result::success(oss.str());
}

} // namespace gtaf::cli
