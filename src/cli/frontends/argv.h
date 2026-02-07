#pragma once

#include "../parser/parser.h"
#include "../executor/executor.h"
#include "../session/session.h"

namespace gtaf::cli {

// ---- Argv Frontend ----

/// @brief Non-interactive (argv) CLI frontend
/// 
/// The ArgvFrontend processes command-line arguments for single-command execution.
/// It creates a new session for each invocation and exits with an appropriate
/// POSIX-compatible exit code. This frontend is suitable for scripting, CI/CD,
/// and automation scenarios.
/// 
/// This frontend uses the shared parser and executor to ensure identical
/// behavior with the REPL frontend as required by ADR-006.
class ArgvFrontend {
public:
    /// @brief Construct with shared parser, executor and session (dependency injection)
    ArgvFrontend(Parser& parser, CommandExecutor& executor, Session& session) noexcept
        : m_parser(parser), m_executor(executor), m_session(session) {}

    /// @brief Deleted default constructor to enforce dependency injection
    ArgvFrontend() = delete;

    /// @brief Destructor
    ~ArgvFrontend() = default;

    // ---- Public Interface ----

    /// @brief Run the CLI with argc/argv arguments
    /// @param argc Argument count from main()
    /// @param argv Argument vector from main()
    /// @return POSIX-compatible exit code (0 for success, non-zero for error)
    int run(int argc, char* argv[]);

private:
    // Use injected/shared components (non-owning)
    Parser& m_parser;
    CommandExecutor& m_executor;
    Session& m_session;
    
    // ---- Output Methods ----
    
    /// @brief Print successful command output to stdout
    /// @param result Result containing success output
    void print_output(const Result& result);
    
    /// @brief Print error messages to stderr
    /// @param result Result containing error information
    void print_error(const Result& result);
};

} // namespace gtaf::cli