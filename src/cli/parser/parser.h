#pragma once

#include "../types.h"
#include <vector>
#include <string>

namespace gtaf::cli {

// ---- Command Parser ----

/// @brief Frontend-agnostic command parser
/// 
/// The Parser is responsible for normalizing all input (argv and REPL) into
/// a canonical Command structure. It handles tokenization, option parsing,
/// and validation while being completely agnostic to the input source.
/// 
/// This ensures identical behavior between argv and REPL modes as required
/// by ADR-006: any input that parses to the same Command structure will
/// produce identical execution results.
class Parser {
public:
    /// @brief Default constructor
    Parser() = default;
    
    /// @brief Destructor
    ~Parser() = default;
    
    // ---- Public Interface ----
    
    /// @brief Parse command from argc/argv (non-interactive mode)
    /// @param argc Argument count from main()
    /// @param argv Argument vector from main()
    /// @return Parsed command structure
    Command parse_argv(int argc, char* argv[]);
    
    /// @brief Parse command from input string (REPL mode)
    /// @param input Raw command line input from user
    /// @return Parsed command structure
    Command parse_string(const std::string& input);
    
private:
    // ---- Tokenization Methods ----
    
    /// @brief Convert argc/argv to token vector
    /// Skips program name (argv[0]) and processes remaining arguments
    std::vector<std::string> tokenize_argv(int argc, char* argv[]);
    
    /// @brief Convert input string to token vector
    /// Simple whitespace-based tokenization
    std::vector<std::string> tokenize_string(const std::string& input);
    
    // ---- Parsing Methods ----
    
    /// @brief Parse token vector into Command structure
    /// Core parsing logic that identifies commands, options, flags, and positionals
    Command parse_tokens(const std::vector<std::string>& tokens);
    
    // ---- Utility Methods ----
    
    /// @brief Check if token is an option (starts with '-')
    /// @param token Token to check
    /// @return True if token is an option or flag
    bool is_option(const std::string& token) const;
    
    /// @brief Check if token is a flag (option without value)
    /// @param token Token to check
    /// @return True if token is a boolean flag
    bool is_flag(const std::string& token) const;
    
    /// @brief Remove option prefix from token
    /// Strips leading '-' or '--' from option names
    /// @param token Option token (e.g., "--verbose", "-v")
    /// @return Option name without prefixes
    std::string strip_option_prefix(const std::string& token) const;
};

} // namespace gtaf::cli