#pragma once

#include "command_base.h"

namespace gtaf::cli {

/// @brief Command to load a GTAF database from file
///
/// Usage: load <file_path> [--verbose]
///
/// Loads atom data from a file into the session's AtomStore.
/// Displays statistics about the loaded data.
class LoadCommand final : public CommandBase {
public:
    [[nodiscard]] std::string name() const override { return "load"; }

    [[nodiscard]] std::string description() const override {
        return "Load database from file: load <path> [--verbose]";
    }

    Result execute(const Command& cmd, Session& session) override;
};

} // namespace gtaf::cli
