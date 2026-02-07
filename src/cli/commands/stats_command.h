#pragma once

#include "command_base.h"

namespace gtaf::cli {

/// @brief Command to display session and system statistics
///
/// Usage: stats [--verbose]
///
/// Shows current session state including output format, verbose mode,
/// and registered commands count.
class StatsCommand final : public CommandBase {
public:
    [[nodiscard]] std::string name() const override { return "stats"; }

    [[nodiscard]] std::string description() const override {
        return "Show session statistics: stats [--verbose]";
    }

    Result execute(const Command& cmd, Session& session) override;
};

} // namespace gtaf::cli
