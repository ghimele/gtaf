#pragma once

#include "../types.h"
#include <string>

namespace gtaf::cli {

// ---- Session Management ----

/// @brief Persistent session state for CLI operations
/// 
/// The Session class encapsulates all long-lived CLI state and resources.
/// It persists across commands in REPL mode and is initialized once in argv mode.
/// This ensures consistency and provides a centralized location for 
/// configuration, connections, and contextual state.
class Session {
public:
    /// @brief Default constructor
    Session() = default;
    
    /// @brief Destructor
    ~Session() = default;
    
    // ---- Public State Members ----
    
    /// @brief Verbose output flag for detailed logging and debugging
    /// When true, commands should provide additional information about their operations
    bool verbose = false;
    
    /// @brief Output format for command results
    /// Controls how command results are formatted and displayed
    OutputFormat output = OutputFormat::Human;
    
    // ---- Public Interface ----
    
    /// @brief Set verbose output mode
    /// @param v True to enable verbose output, false to disable
    void set_verbose(bool v) { verbose = v; }
    
    /// @brief Set the output format for command results
    /// @param format The desired output format (human, json, csv)
    void set_output_format(OutputFormat format) { output = format; }
    
private:
    // ---- Future Extensions ----
    // These are placeholders for future session capabilities
    
    // DatabaseConnection db;              ///< Persistent database connection
    // std::string current_project;        ///< Current working project context
    // std::map<std::string, std::string> environment;  ///< Environment variables
    // std::string working_directory;      ///< Current working directory
};

} // namespace gtaf::cli