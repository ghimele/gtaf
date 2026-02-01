#pragma once

#include "../parser/parser.h"
#include "../executor/executor.h"
#include "../session/session.h"
#include <string>

namespace gtaf::cli {

// ---- REPL Frontend ----

/// @brief Interactive Read-Eval-Print Loop frontend
/// 
/// The ReplFrontend provides an interactive command session where users can
/// execute multiple commands within a persistent session. It maintains state
/// across commands and provides a continuous user experience similar to
/// database CLI tools like SQLite.
/// 
/// This frontend uses the same shared parser and executor as the argv frontend
/// to ensure identical behavior as required by ADR-006.
class ReplFrontend {
public:
    /// @brief Default constructor
    ReplFrontend() = default;
    
    /// @brief Destructor
    ~ReplFrontend() = default;
    
    // ---- Public Interface ----
    
    /// @brief Start the interactive REPL session
    /// Runs until user enters an exit command
    void run();
    
private:
    // ---- Shared Components ----
    
    /// @brief Parser for converting user input to Command structure
    Parser m_parser;
    
    /// @brief Executor for command dispatch and execution
    CommandExecutor m_executor;
    
    /// @brief Persistent session state across multiple commands
    Session m_session;
    
    // ---- REPL Interaction Methods ----
    
    /// @brief Read a line of input from the user
    /// @return User input string (may be empty)
    std::string read_line();
    
    /// @brief Display the interactive prompt
    void print_prompt();
    
    /// @brief Print successful command output
    /// @param result Result containing success output
    void print_output(const Result& result);
    
    /// @brief Print error messages (non-fatal, REPL continues)
    /// @param result Result containing error information
    void print_error(const Result& result);
    
    /// @brief Check if command should terminate the REPL session
    /// @param cmd Parsed command to check
    /// @return True if command is an exit command
    bool should_exit(const Command& cmd) const;
    
    // ---- Exit Command Recognition ----
    
    /// @brief List of commands that terminate the REPL session
    static constexpr const char* EXIT_COMMANDS[] = {"exit", "quit", "q"};
};

} // namespace gtaf::cli