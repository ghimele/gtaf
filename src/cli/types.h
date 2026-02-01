#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <functional>

namespace gtaf::cli {

// ---- Output Format Enumeration ----

/// @brief Supported output formats for CLI commands
/// Used to control how command results are displayed to users
enum class OutputFormat {
    Human,  ///< Human-readable format with descriptions
    Json,   ///< JSON format for machine consumption
    Csv     ///< CSV format for data export
};

// ---- Command Structure ----

/// @brief Canonical representation of a parsed CLI command
/// This structure is the output of the parser and input to the executor.
/// It provides a frontend-agnostic representation of user commands.
struct Command {
    std::string name;                                          ///< Command name (e.g., "help", "verbose")
    std::vector<std::string> positionals;                      ///< Positional arguments in order
    std::unordered_map<std::string, std::string> options;    ///< Options with values (e.g., "--format=json")
    std::unordered_set<std::string> flags;                    ///< Boolean flags (e.g., "--verbose", "-v")
};

// ---- Result Structure ----

/// @brief Result of command execution
/// Encapsulates both success/failure status and associated output or error messages
struct Result {
    int exit_code;           ///< Process exit code (0 for success, non-zero for error)
    std::string output;     ///< Standard output content for successful commands
    std::string error;      ///< Error message content for failed commands
    
    /// @brief Create a successful result
    /// @param output Optional output message
    /// @return Result with exit_code=0 and provided output
    static Result success(const std::string& output = "") {
        return {0, output, ""};
    }
    
    /// @brief Create a failure result
    /// @param error_msg Error message describing the failure
    /// @param exit_code Optional custom exit code (defaults to 1)
    /// @return Result with non-zero exit code and provided error message
    static Result failure(const std::string& error_msg, int exit_code = 1) {
        return {exit_code, "", error_msg};
    }
};

// ---- Command Handler Type ----

/// @brief Function signature for command handlers
/// Command handlers implement the actual business logic for each CLI command
/// @param cmd The parsed command structure
/// @param session Current session context for state access
/// @return Result of command execution
using CommandHandler = std::function<Result(const Command&, class Session&)>;

} // namespace gtaf::cli