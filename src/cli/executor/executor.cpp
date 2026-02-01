#include "executor.h"
#include <sstream>
#include <algorithm>

namespace gtaf::cli {

// ---- Constructor ----

CommandExecutor::CommandExecutor() {
    register_builtin_commands();
}

// ---- Public Methods ----

Result CommandExecutor::execute(const Command& cmd, Session& session) {
    // Validate command name
    if (cmd.name.empty()) {
        return Result::failure("No command provided");
    }
    
    // Find registered handler for the command
    auto it = m_handlers.find(cmd.name);
    if (it == m_handlers.end()) {
        std::ostringstream oss;
        oss << "Unknown command: '" << cmd.name << "'";
        return Result::failure(oss.str());
    }
    
    // Execute the handler with exception safety
    try {
        return it->second(cmd, session);
    } catch (const std::exception& e) {
        std::ostringstream oss;
        oss << "Error executing command '" << cmd.name << "': " << e.what();
        return Result::failure(oss.str());
    }
}

void CommandExecutor::register_command(const std::string& name, CommandHandler handler) {
    // Move handler into registry to avoid copying
    m_handlers[name] = std::move(handler);
}

std::vector<std::string> CommandExecutor::get_registered_commands() const {
    std::vector<std::string> commands;
    
    // Extract command names from registry
    for (const auto& pair : m_handlers) {
        commands.push_back(pair.first);
    }
    
    // Sort for consistent ordering in help output
    std::sort(commands.begin(), commands.end());
    return commands;
}

// ---- Initialization ----

void CommandExecutor::register_builtin_commands() {
    // Register built-in commands using lambda captures to member methods
    // This pattern ensures proper lifetime management and clean delegation
    
    register_command("help", [this](const Command& cmd, Session& session) {
        return handle_help(cmd, session);
    });
    
    register_command("verbose", [this](const Command& cmd, Session& session) {
        return handle_verbose(cmd, session);
    });
    
    register_command("format", [this](const Command& cmd, Session& session) {
        return handle_format(cmd, session);
    });
}

// ---- Built-in Command Handlers ----

Result CommandExecutor::handle_help(const Command& cmd, Session& session) {
    std::ostringstream oss;
    oss << "GTAF CLI - Available commands:\n\n";
    
    // Get sorted list of all commands
    auto commands = get_registered_commands();
    for (const auto& command : commands) {
        oss << "  " << command;
        
        // Add brief descriptions for built-in commands
        // Future: command descriptions could be stored with handlers
        if (command == "help") {
            oss << " - Show this help message";
        } else if (command == "verbose") {
            oss << " - Toggle verbose output";
        } else if (command == "format") {
            oss << " - Set output format (human|json|csv)";
        }
        oss << "\n";
    }
    
    return Result::success(oss.str());
}

Result CommandExecutor::handle_verbose(const Command& cmd, Session& session) {
    // Support explicit on/off flags
    if (cmd.flags.count("on") || cmd.flags.count("true")) {
        session.set_verbose(true);
        return Result::success("Verbose output enabled");
    } else if (cmd.flags.count("off") || cmd.flags.count("false")) {
        session.set_verbose(false);
        return Result::success("Verbose output disabled");
    } else {
        // Default behavior: toggle current state
        session.set_verbose(!session.verbose);
        std::ostringstream oss;
        oss << "Verbose output " << (session.verbose ? "enabled" : "disabled");
        return Result::success(oss.str());
    }
}

Result CommandExecutor::handle_format(const Command& cmd, Session& session) {
    // If no argument provided, show current format
    if (cmd.positionals.empty()) {
        std::ostringstream oss;
        oss << "Current format: ";
        switch (session.output) {
            case OutputFormat::Human: oss << "human"; break;
            case OutputFormat::Json: oss << "json"; break;
            case OutputFormat::Csv: oss << "csv"; break;
        }
        return Result::success(oss.str());
    }
    
    // Parse and set new format
    const std::string& format_str = cmd.positionals[0];
    if (format_str == "human") {
        session.set_output_format(OutputFormat::Human);
        return Result::success("Output format set to human");
    } else if (format_str == "json") {
        session.set_output_format(OutputFormat::Json);
        return Result::success("Output format set to json");
    } else if (format_str == "csv") {
        session.set_output_format(OutputFormat::Csv);
        return Result::success("Output format set to csv");
    } else {
        return Result::failure("Invalid format. Use: human, json, or csv");
    }
}

} // namespace gtaf::cli