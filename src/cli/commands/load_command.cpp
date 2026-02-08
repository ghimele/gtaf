#include "load_command.h"
#include <sstream>
#include <iomanip>
#include <chrono>

namespace gtaf::cli {

Result LoadCommand::execute(const Command& cmd, Session& session) {
    // Validate that a file path was provided
    if (cmd.positionals.empty()) {
        return Result::failure("Usage: " + description());
    }

    const std::string& file_path = cmd.positionals[0];

    // Check if verbose flag is set
    bool verbose = session.verbose || cmd.flags.count("verbose") || cmd.flags.count("v");

    std::ostringstream oss;

    if (verbose) {
        oss << "Loading data from: " << file_path << "\n";
    }

    // Get the session's atom store and load data
    auto& store = session.get_store();

    auto start_time = std::chrono::high_resolution_clock::now();

    if (!store.load(file_path)) {
        std::ostringstream err;
        err << "Failed to load data file: " << file_path;
        return Result::failure(err.str());
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    // Get and display statistics
    auto stats = store.get_stats();

    oss << "Successfully loaded " << stats.total_atoms << " atoms in "
        << duration.count() << "ms\n";

    if (verbose) {
        oss << "\n=== Store Statistics ===\n"
            << "Total atoms:           " << std::setw(12) << stats.total_atoms << "\n"
            << "Canonical atoms:       " << std::setw(12) << stats.canonical_atoms << "\n"
            << "Unique canonical atoms:" << std::setw(12) << stats.unique_canonical_atoms << "\n"
            << "Total references:      " << std::setw(12) << stats.total_references << "\n"
            << "Total entities:        " << std::setw(12) << stats.total_entities << "\n";

        if (stats.canonical_atoms > 0) {
            double dedup_ratio = static_cast<double>(stats.unique_canonical_atoms) / stats.canonical_atoms;
            oss << "Deduplication ratio:  " << std::setw(12) << std::fixed << std::setprecision(3)
                << dedup_ratio << " (lower is better)\n";
        }
    }

    return Result::success(oss.str());
}

} // namespace gtaf::cli
