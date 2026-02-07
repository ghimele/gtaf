#pragma once

#include "command_base.h"

namespace gtaf::cli {

/// @brief Command to save session data to file
///
/// Usage: save <file_path> [--verbose]
///
/// Saves the current session's AtomStore data to the specified file.
/// Provides timing information and statistics about the saved data.
class SaveCommand final : public CommandBase {
public:
    [[nodiscard]] std::string name() const override { return "save"; }

    [[nodiscard]] std::string description() const override {
        return "Save database to file: save <path> [--verbose]";
    }

    Result execute(const Command& cmd, Session& session) override;
};

} // namespace gtaf::cli