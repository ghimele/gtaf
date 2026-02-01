#include "argv.h"
#include <iostream>

namespace gtaf::cli {

// ---- Main Execution Method ----

int ArgvFrontend::run(int argc, char* argv[]) {
    // Parse command-line arguments using shared parser
    auto cmd = m_parser.parse_argv(argc, argv);
    
    // Handle case where no command is provided - show help
    if (cmd.name.empty()) {
        auto result = m_executor.execute(Command{"help"}, m_session);
        print_output(result);
        return result.exit_code;
    }
    
    // Execute the command using shared executor
    auto result = m_executor.execute(cmd, m_session);
    
    // Route output based on success/failure
    if (result.exit_code == 0) {
        print_output(result);
    } else {
        print_error(result);
    }
    
    // Return POSIX-compatible exit code
    return result.exit_code;
}

// ---- Output Methods Implementation ----

void ArgvFrontend::print_output(const Result& result) {
    // Only print if there's actual content to output
    if (!result.output.empty()) {
        std::cout << result.output << std::endl;
    }
}

void ArgvFrontend::print_error(const Result& result) {
    // Only print if there's an error message
    if (!result.error.empty()) {
        // Use stderr for error messages as per Unix conventions
        std::cerr << "Error: " << result.error << std::endl;
    }
}

} // namespace gtaf::cli