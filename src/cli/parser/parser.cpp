#include "parser.h"
#include <sstream>
#include <algorithm>
#include <cctype>

namespace gtaf::cli {

// ---- Public Parsing Methods ----

Command Parser::parse_argv(int argc, char* argv[]) {
    auto tokens = tokenize_argv(argc, argv);
    return parse_tokens(tokens);
}

Command Parser::parse_string(const std::string& input) {
    auto tokens = tokenize_string(input);
    return parse_tokens(tokens);
}

// ---- Tokenization Implementation ----

std::vector<std::string> Parser::tokenize_argv(int argc, char* argv[]) {
    std::vector<std::string> tokens;
    
    // Skip argv[0] (program name) and process remaining arguments
    for (int i = 1; i < argc; ++i) {
        tokens.emplace_back(argv[i]);
    }
    
    return tokens;
}

std::vector<std::string> Parser::tokenize_string(const std::string& input) {
    std::vector<std::string> tokens;
    std::string current_token;
    bool in_single_quote = false;
    bool in_double_quote = false;
    bool escape_next = false;
    bool token_started = false;  // Track if we've started a token (for empty quotes)

    for (size_t i = 0; i < input.size(); ++i) {
        char c = input[i];

        // Handle escape sequences
        if (escape_next) {
            current_token += c;
            token_started = true;
            escape_next = false;
            continue;
        }

        // Backslash escapes next character (only outside single quotes)
        if (c == '\\' && !in_single_quote) {
            escape_next = true;
            continue;
        }

        // Handle single quotes (no escape processing inside)
        if (c == '\'' && !in_double_quote) {
            in_single_quote = !in_single_quote;
            token_started = true;  // Opening quote starts a token
            continue;
        }

        // Handle double quotes
        if (c == '"' && !in_single_quote) {
            in_double_quote = !in_double_quote;
            token_started = true;  // Opening quote starts a token
            continue;
        }

        // Whitespace outside quotes ends current token
        if (std::isspace(static_cast<unsigned char>(c)) && !in_single_quote && !in_double_quote) {
            if (token_started) {
                tokens.emplace_back(std::move(current_token));
                current_token.clear();
                token_started = false;
            }
            continue;
        }

        // Regular character - add to current token
        current_token += c;
        token_started = true;
    }

    // Don't forget the last token
    if (token_started) {
        tokens.emplace_back(std::move(current_token));
    }

    return tokens;
}

// ---- Core Parsing Logic ----

Command Parser::parse_tokens(const std::vector<std::string>& tokens) {
    Command cmd;
    
    // Empty input returns empty command
    if (tokens.empty()) {
        return cmd;
    }
    
    // First token is always the command name
    cmd.name = tokens[0];
    
    // Process remaining tokens as options, flags, or positionals
    for (size_t i = 1; i < tokens.size(); ++i) {
        const std::string& token = tokens[i];
        
        if (is_option(token)) {
            // Handle options (with values) and flags (boolean)
            auto option_name = strip_option_prefix(token);

            // Check for inline --option=value syntax
            auto eq_pos = option_name.find('=');
            if (eq_pos != std::string::npos) {
                // Inline value: --format=json -> options["format"] = "json"
                auto value = option_name.substr(eq_pos + 1);
                option_name = option_name.substr(0, eq_pos);
                cmd.options[option_name] = value;
            } else if (i + 1 < tokens.size() && !is_option(tokens[i + 1])) {
                // Option with value: --format json
                cmd.options[option_name] = tokens[i + 1];
                ++i; // Skip the value token
            } else {
                // Boolean flag: --verbose or -v
                cmd.flags.insert(option_name);
            }
        } else {
            // Positional argument
            cmd.positionals.push_back(token);
        }
    }
    
    return cmd;
}

// ---- Utility Methods Implementation ----

bool Parser::is_option(const std::string& token) const {
    // Option must start with '-' and have at least one more character
    return token.length() >= 2 && token[0] == '-';
}

bool Parser::is_flag(const std::string& token) const {
    // A flag is an option that doesn't contain '=' (for inline option=value syntax)
    return is_option(token) && token.find('=') == std::string::npos;
}

std::string Parser::strip_option_prefix(const std::string& token) const {
    // Remove leading '-' or '--' from option names
    if (token.length() >= 2 && token[0] == '-') {
        if (token.length() >= 3 && token[1] == '-') {
            // Long option: --verbose -> verbose
            return token.substr(2);
        } else {
            // Short option: -v -> v
            return token.substr(1);
        }
    }
    return token; // Fallback, though this shouldn't happen for valid options
}

} // namespace gtaf::cli