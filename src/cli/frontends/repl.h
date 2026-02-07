#pragma once

#include "../parser/parser.h"
#include "../executor/executor.h"
#include "../session/session.h"
#include <string>
#include <array>

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
	/// @brief Construct with shared parser, executor and session.
	ReplFrontend(Parser& parser, CommandExecutor& executor, Session& session) noexcept
		: m_parser(parser), m_executor(executor), m_session(session) {}

	/// @brief Deleted default constructor to enforce dependency injection
	ReplFrontend() = delete;

	/// @brief Destructor
	~ReplFrontend() = default;

	// ---- Public Interface ----
	
	/// @brief Start the interactive REPL session
	/// Runs until user enters an exit command
	void run();
	
	/// @brief Accessor for the last command's exit code
	/// @return Last exit code (0-255) from the executed commands
	int exit_code() const noexcept { return m_last_exit_code; }
	
private:
	// Use injected/shared components (non-owning)
	Parser& m_parser;
	CommandExecutor& m_executor;
	Session& m_session;

	int m_last_exit_code = 0;
	
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
	// Use C++20 inline constexpr array for in-class initialization
	static inline constexpr std::array<const char*, 3> EXIT_COMMANDS = {"exit", "quit", "q"};
};

} // namespace gtaf::cli