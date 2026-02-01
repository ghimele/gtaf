#pragma once

#include "../types.h"
#include "../session/session.h"
#include <unordered_map>
#include <string>

namespace gtaf::cli {

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
    
    /// @brief Register a new command handler
    /// @param name Command name to register
    /// @param handler Function that implements the command logic
    void register_command(const std::string& name, CommandHandler handler);
    
    /// @brief Get list of all registered command names
    /// @return Sorted vector of available command names
    std::vector<std::string> get_registered_commands() const;
    
private:
    // ---- Private Members ----
    
    /// @brief Registry mapping command names to their handlers
    std::unordered_map<std::string, CommandHandler> m_handlers;
    
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