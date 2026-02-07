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

    // Find registered command
    auto it = m_commands.find(cmd.name);
    if (it == m_commands.end()) {
        std::ostringstream oss;
        oss << "Unknown command: '" << cmd.name << "'";
        return Result::failure(oss.str());
    }

    // Execute the handler with exception safety
    try {
        return it->second.handler(cmd, session);
    } catch (const std::exception& e) {
        std::ostringstream oss;
        oss << "Error executing command '" << cmd.name << "': " << e.what();
        return Result::failure(oss.str());
    }
}

void CommandExecutor::register_command(const std::string& name, const std::string& description, CommandHandler handler) {
    m_commands[name] = CommandInfo{std::move(handler), description};
}

void CommandExecutor::register_command(std::shared_ptr<CommandBase> command) {
    // Capture shared_ptr by value to ensure command object lifetime
    auto cmd_ptr = command;
    register_command(
        command->name(),
        command->description(),
        [cmd_ptr](const Command& cmd, Session& session) {
            return cmd_ptr->execute(cmd, session);
        }
    );
}

std::vector<std::string> CommandExecutor::get_registered_commands() const {
    std::vector<std::string> commands;

    // Extract command names from registry
    for (const auto& [name, info] : m_commands) {
        commands.push_back(name);
    }

    // Sort for consistent ordering in help output
    std::sort(commands.begin(), commands.end());
    return commands;
}

std::string CommandExecutor::get_command_description(const std::string& name) const {
    auto it = m_commands.find(name);
    if (it != m_commands.end()) {
        return it->second.description;
    }
    return "";
}

// ---- Initialization ----

void CommandExecutor::register_builtin_commands() {
    // Register built-in commands with descriptions
    // Descriptions are stored alongside handlers for use in help output

    register_command("help", "Show available commands and their descriptions",
        [this](const Command& cmd, Session& session) {
            return handle_help(cmd, session);
        });

    register_command("verbose", "Toggle verbose output (use --on/--off for explicit control)",
        [this](const Command& cmd, Session& session) {
            return handle_verbose(cmd, session);
        });

    register_command("format", "Set output format: format <human|json|csv>",
        [this](const Command& cmd, Session& session) {
            return handle_format(cmd, session);
        });
}

// ---- Built-in Command Handlers ----

Result CommandExecutor::handle_help(const Command& cmd, Session& session) {
    std::ostringstream oss;
    oss << "GTAF CLI - Available commands:\n\n";

    // Get sorted list of all commands and their descriptions
    auto commands = get_registered_commands();
    for (const auto& command : commands) {
        oss << "  " << command;

        auto description = get_command_description(command);
        if (!description.empty()) {
            oss << " - " << description;
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