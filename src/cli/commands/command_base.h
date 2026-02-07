#pragma once

#include "../types.h"
#include "../session/session.h"
#include <string>

namespace gtaf::cli {

// ---- Command Base Class ----

/// @brief Abstract base class for CLI commands
///
/// Derive from this class to create new commands. Each command provides:
/// - A name (used for invocation)
/// - A description (shown in help output)
/// - An execute method (implements the command logic)
///
/// Commands are registered with the CommandExecutor and can be invoked
/// from both argv and REPL modes with identical behavior (ADR-006).
class CommandBase {
public:
    virtual ~CommandBase() = default;

    /// @brief Get the command name (used for invocation)
    /// @return Command name (e.g., "stats", "save")
    [[nodiscard]] virtual std::string name() const = 0;

    /// @brief Get the command description (shown in help)
    /// @return Brief description with optional usage hints
    [[nodiscard]] virtual std::string description() const = 0;

    /// @brief Execute the command
    /// @param cmd Parsed command with positionals, options, and flags
    /// @param session Current session state
    /// @return Result containing exit code and output/error
    virtual Result execute(const Command& cmd, Session& session) = 0;
};

} // namespace gtaf::cli
