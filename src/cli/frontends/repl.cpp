#include "repl.h"
#include <iostream>
#include <algorithm>

namespace gtaf::cli {

// ---- Main REPL Loop ----

void ReplFrontend::run() {
    // Display welcome message and basic usage information
    std::cout << "GTAF CLI - Interactive Mode" << std::endl;
    std::cout << "Type 'help' for available commands or 'exit' to quit." << std::endl << std::endl;
    
    // Main REPL loop - continues until exit command or EOF
    while (true) {
        print_prompt();
        
        // Read user input
        std::string input = read_line();

        // If user sent EOF (e.g., Ctrl-D), exit REPL
        if (std::cin.eof()) {
            std::cout << "Goodbye!" << std::endl;
            break;
        }

        if (input.empty()) {
            // Empty input - just show prompt again
            continue;
        }
        
        // Parse input using shared parser
        auto cmd = m_parser.parse_string(input);
        
        // Check for exit commands
        if (should_exit(cmd)) {
            std::cout << "Goodbye!" << std::endl;
            break;
        }
        
        // Execute command using shared executor with persistent session
        auto result = m_executor.execute(cmd, m_session);

        // Store last exit code, keep it in 0..255 for POSIX compatibility
        m_last_exit_code = (result.exit_code & 0xFF);

        // Display results based on success/failure
        // Note: Errors in REPL don't terminate the session
        if (m_last_exit_code == 0) {
            print_output(result);
        } else {
            print_error(result);
        }
    }
}

// ---- REPL Interaction Methods ----

std::string ReplFrontend::read_line() {
    std::string line;
    std::getline(std::cin, line);
    return line;
}

void ReplFrontend::print_prompt() {
    // Show contextual prompt with flush for immediate display
    std::cout << "gtaf> ";
    std::cout.flush();
}

void ReplFrontend::print_output(const Result& result) {
    // Print successful output to stdout
    if (!result.output.empty()) {
        std::cout << result.output << std::endl;
    }
}

void ReplFrontend::print_error(const Result& result) {
    // Print errors to stderr but continue REPL session
    if (!result.error.empty()) {
        std::cerr << "Error: " << result.error << std::endl;
    }
}

// ---- Exit Command Detection ----

bool ReplFrontend::should_exit(const Command& cmd) const {
    // Check if command name matches any exit command
    return std::find(std::begin(EXIT_COMMANDS), std::end(EXIT_COMMANDS), cmd.name) != std::end(EXIT_COMMANDS);
}

} // namespace gtaf::cli