// ---- GTAF CLI Main Entry Point ----
// 
// This file serves as the main entry point for the GTAF CLI application.
// It routes to either the interactive REPL frontend or the non-interactive
// argv frontend based on command-line arguments.
// 
// This routing logic ensures both frontends can use the same shared components
// (parser, executor, session) to maintain behavioral consistency as required
// by ADR-006.

#include "frontends/argv.h"
#include "frontends/repl.h"
#include <iostream>
#include <string>
#include <algorithm>

int main(int argc, char* argv[]) {
    // Check for explicit REPL mode requests
    if (argc > 1) {
        std::string first_arg = argv[1];
        
        // Normalize argument to lowercase for case-insensitive comparison
        std::transform(first_arg.begin(), first_arg.end(), first_arg.begin(), ::tolower);
        
        // Check for REPL mode indicators
        if (first_arg == "repl" || first_arg == "interactive" || first_arg == "-i") {
            gtaf::cli::ReplFrontend repl;
            repl.run();
            return 0; // Successful REPL session
        }
    }
    
    // Handle no-argument case: show usage information
    if (argc == 1) {
        std::cout << "GTAF CLI" << std::endl;
        std::cout << "Usage: gtaf <command> [options]   # Non-interactive mode" << std::endl;
        std::cout << "       gtaf repl                  # Interactive mode" << std::endl;
        std::cout << "       gtaf help                   # Show available commands" << std::endl;
        return 0; // Successful help display
    }
    
    // Default: treat as non-interactive command execution
    // Use argv frontend for single-command execution
    gtaf::cli::ArgvFrontend argv_frontend;
    return argv_frontend.run(argc, argv);
}