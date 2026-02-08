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
#include "commands/stats_command.h"
#include "commands/load_command.h"
#include "commands/save_command.h"
#include "commands/import_command.h"
#include <iostream>
#include <string>
#include <algorithm>
#include <memory>

namespace {

/// @brief Register all external commands with the executor
/// Add new command registrations here
void register_commands(gtaf::cli::CommandExecutor& executor) {
    // Register external commands (add new commands here)
    executor.register_command(std::make_shared<gtaf::cli::LoadCommand>());
    executor.register_command(std::make_shared<gtaf::cli::SaveCommand>());
    executor.register_command(std::make_shared<gtaf::cli::ImportCsvCommand>());
    // executor.register_command(std::make_shared<gtaf::cli::StatsCommand>());
}

/// @brief Show basic usage information
void show_usage() {
    std::cout << "GTAF CLI" << std::endl;
    std::cout << "Usage: gtaf <command> [options]   # Non-interactive mode" << std::endl;
    std::cout << "       gtaf repl                  # Interactive mode" << std::endl;
    std::cout << "       gtaf help                  # Show available commands" << std::endl;
}

} // anonymous namespace

int main(int argc, char* argv[]) {
    // Handle no-argument case: show usage information
    if (argc == 1) {
        show_usage();
        return 0;
    }

    std::string first_arg = argv[1];

    // Normalize argument to lowercase for case-insensitive comparison
    std::transform(first_arg.begin(), first_arg.end(), first_arg.begin(), ::tolower);

    // Handle -h/--help flags with basic usage (common CLI convention)
    if (first_arg == "-h" || first_arg == "--help") {
        show_usage();
        return 0;
    }

    // Create shared components
    gtaf::cli::Parser parser;
    gtaf::cli::CommandExecutor executor;
    gtaf::cli::Session session;

    // Register external commands
    register_commands(executor);

    // Check for REPL mode indicators
    if (first_arg == "repl" || first_arg == "interactive" || first_arg == "-i") {
        gtaf::cli::ReplFrontend repl(parser, executor, session);
        repl.run();
        return repl.exit_code();
    }

    // Default: treat as non-interactive command execution
    gtaf::cli::ArgvFrontend argv_frontend(parser, executor, session);
    return argv_frontend.run(argc, argv);
}