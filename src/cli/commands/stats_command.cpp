#include "stats_command.h"
#include <sstream>

namespace gtaf::cli {

Result StatsCommand::execute(const Command& cmd, Session& session) {
    std::ostringstream oss;

    oss << "Session Statistics:\n";
    oss << "  Verbose mode: " << (session.verbose ? "enabled" : "disabled") << "\n";

    oss << "  Output format: ";
    switch (session.output) {
        case OutputFormat::Human: oss << "human"; break;
        case OutputFormat::Json: oss << "json"; break;
        case OutputFormat::Csv: oss << "csv"; break;
    }
    oss << "\n";

    // Add more stats here as needed (e.g., loaded database info, query count, etc.)

    return Result::success(oss.str());
}

} // namespace gtaf::cli
