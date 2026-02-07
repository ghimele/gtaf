#pragma once

#include "../types.h"
#include "../session/session.h"
#include "../commands/command_base.h"
#include <unordered_map>
#include <string>
#include <memory>

namespace gtaf::cli {

// ---- Command Information ----

/// @brief Metadata for a registered command
/// Stores the handler function along with its description for help output
struct CommandInfo {
    CommandHandler handler;      ///< Function that implements the command logic
    std::string description;     ///< Brief description shown in help output
};

// ---- Command Executor ----

/// @brief Central command dispatcher and handler registry
///
/// The CommandExecutor is responsible for dispatching parsed commands to
/// appropriate handlers, managing handler registration, and ensuring
/// consistent execution behavior across all frontends.
///
/// As mandated by ADR-006, this class provides a single execution pipeline
/// that guarantees identical behavior between argv and REPL modes.
class CommandExecutor {
public:
    /// @brief Constructor - registers built-in commands
    CommandExecutor();

    /// @brief Destructor
    ~CommandExecutor() = default;

    // ---- Public Interface ----

    /// @brief Execute a command within the given session context
    /// @param cmd The parsed command to execute
    /// @param session Current session state for context and persistence
    /// @return Result containing exit code, output, and error information
    Result execute(const Command& cmd, Session& session);

    /// @brief Register a new command handler with description
    /// @param name Command name to register
    /// @param description Brief description for help output
    /// @param handler Function that implements the command logic
    void register_command(const std::string& name, const std::string& description, CommandHandler handler);

    /// @brief Register a command object (preferred for complex commands)
    /// @param command Shared pointer to a CommandBase-derived object
    /// The executor takes shared ownership of the command object
    void register_command(std::shared_ptr<CommandBase> command);

    /// @brief Get list of all registered command names
    /// @return Sorted vector of available command names
    std::vector<std::string> get_registered_commands() const;

    /// @brief Get description for a registered command
    /// @param name Command name to look up
    /// @return Description string, or empty if command not found
    std::string get_command_description(const std::string& name) const;

private:
    // ---- Private Members ----

    /// @brief Registry mapping command names to their info (handler + description)
    std::unordered_map<std::string, CommandInfo> m_commands;
    
    // ---- Initialization Methods ----
    
    /// @brief Register all built-in CLI commands
    /// Called during construction to set up basic functionality
    void register_builtin_commands();
    
    // ---- Built-in Command Handlers ----

    /// @brief Handle the 'help' command - shows available commands
    Result handle_help(const Command& cmd, Session& session);

    /// @brief Handle the 'verbose' command - toggles verbose output
    Result handle_verbose(const Command& cmd, Session& session);

    /// @brief Handle the 'format' command - sets output format
    Result handle_format(const Command& cmd, Session& session);
};

} // namespace gtaf::cli