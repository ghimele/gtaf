#include "save_command.h"
#include <sstream>
#include <iomanip>
#include <chrono>

namespace gtaf::cli {

Result SaveCommand::execute(const Command& cmd, Session& session) {
    // Validate that a file path was provided
    if (cmd.positionals.empty()) {
        return Result::failure("Usage: " + description());
    }

    const std::string& file_path = cmd.positionals[0];

    // Check if verbose flag is set
    bool verbose = session.verbose || cmd.flags.count("verbose") || cmd.flags.count("v");

    // Check if store has data to save
    if (!session.has_store()) {
        return Result::failure("No data loaded - use 'load' command first");
    }

    std::ostringstream oss;

    if (verbose) {
        oss << "Saving data to: " << file_path << "\n";
    }

    // Get the session's atom store and save data
    const auto& store = session.get_store();

    auto start_time = std::chrono::high_resolution_clock::now();

    if (!store.save(file_path)) {
        std::ostringstream err;
        err << "Failed to save data file: " << file_path;
        return Result::failure(err.str());
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    // Get and display statistics
    auto stats = store.get_stats();

    oss << "Successfully saved " << stats.total_atoms << " atoms to " << file_path << " in "
        << duration.count() << "ms\n";

    if (verbose) {
        oss << "\n=== Saved Data Statistics ===\n"
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