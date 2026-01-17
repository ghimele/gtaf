#include "../core/atom_log.h"
#include "../core/projection_engine.h"
#include "../types/hash_utils.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <regex>

using namespace gtaf;

// Helper function to parse SQL INSERT values
std::vector<std::string> parse_insert_values(const std::string& line) {
    std::vector<std::string> values;

    // Find the values clause: "values (...)"
    size_t values_pos = line.find("values (");
    if (values_pos == std::string::npos) {
        return values;
    }

    // Extract the content between parentheses
    size_t start = values_pos + 8; // After "values ("
    size_t end = line.rfind(");");
    if (end == std::string::npos) {
        end = line.rfind(")");
    }

    if (end == std::string::npos || start >= end) {
        return values;
    }

    std::string values_str = line.substr(start, end - start);

    // Parse comma-separated values, handling strings with commas
    std::string current;
    bool in_string = false;
    bool in_function = false;
    int paren_depth = 0;

    for (size_t i = 0; i < values_str.length(); ++i) {
        char c = values_str[i];

        if (c == '\'') {
            in_string = !in_string;
            current += c;
        } else if (!in_string) {
            if (c == '(') {
                paren_depth++;
                in_function = true;
                current += c;
            } else if (c == ')') {
                paren_depth--;
                if (paren_depth == 0) {
                    in_function = false;
                }
                current += c;
            } else if (c == ',' && !in_function) {
                // End of value
                values.push_back(current);
                current.clear();
            } else {
                current += c;
            }
        } else {
            current += c;
        }
    }

    // Add the last value
    if (!current.empty()) {
        values.push_back(current);
    }

    return values;
}

// Helper to clean and extract string value
std::string extract_string(const std::string& value) {
    std::string trimmed = value;

    // Remove leading/trailing whitespace
    trimmed.erase(0, trimmed.find_first_not_of(" \t\n\r"));
    trimmed.erase(trimmed.find_last_not_of(" \t\n\r") + 1);

    // Handle NULL (case-insensitive)
    std::string lower_trimmed = trimmed;
    std::transform(lower_trimmed.begin(), lower_trimmed.end(), lower_trimmed.begin(), ::tolower);
    if (lower_trimmed == "null" || trimmed.empty()) {
        return "";
    }

    // Remove quotes
    if (trimmed.length() >= 2 && trimmed[0] == '\'' && trimmed[trimmed.length()-1] == '\'') {
        return trimmed.substr(1, trimmed.length() - 2);
    }

    return trimmed;
}

// Parse column names from INSERT statement
std::vector<std::string> parse_column_names(const std::string& line) {
    std::vector<std::string> columns;

    // Find the column list: "(...) values"
    size_t start = line.find('(');
    size_t end = line.find(") values");

    if (start == std::string::npos || end == std::string::npos) {
        return columns;
    }

    std::string cols_str = line.substr(start + 1, end - start - 1);

    // Split by comma
    std::stringstream ss(cols_str);
    std::string col;
    while (std::getline(ss, col, ',')) {
        // Trim whitespace
        col.erase(0, col.find_first_not_of(" \t\n\r"));
        col.erase(col.find_last_not_of(" \t\n\r") + 1);
        columns.push_back(col);
    }

    return columns;
}

// Create EntityId from integer ID
types::EntityId create_entity_id(int64_t id) {
    types::EntityId entity{};
    std::fill(entity.bytes.begin(), entity.bytes.end(), 0);

    // Store the ID in the first 8 bytes (little-endian)
    for (int i = 0; i < 8; ++i) {
        entity.bytes[i] = static_cast<uint8_t>((id >> (i * 8)) & 0xFF);
    }

    return entity;
}

int main(int argc, char* argv[]) {
    std::cout << "=== GTAF SQL Import - WORKREQUEST Data ===\n\n";

    // Determine input file
    std::string sql_file = "../test_data/export_WORKREQUEST.sql";
    if (argc > 1) {
        sql_file = argv[1];
    }

    std::cout << "Reading SQL file: " << sql_file << "\n\n";

    std::ifstream infile(sql_file);
    if (!infile.is_open()) {
        std::cerr << "Error: Could not open file: " << sql_file << "\n";
        return 1;
    }

    core::AtomLog log;
    std::vector<std::string> column_names;
    int record_count = 0;
    int line_number = 0;

    std::string current_statement;
    std::string line;
    bool in_insert = false;

    size_t entity_id = 0;
    while (std::getline(infile, line)) {
        line_number++;

        // Skip comments when not in an INSERT
        if (!in_insert && (line.empty() || line[0] == '-' || line.substr(0, 3) == "REM" ||
            line.substr(0, 3) == "SET" || line.substr(0, 4) == "    ")) {
            continue;
        }

        // Check if this line starts an INSERT statement
        if (line.find("Insert into") != std::string::npos) {
            in_insert = true;
            current_statement = line;
        } else if (in_insert) {
            // Accumulate multi-line INSERT statement
            current_statement += " " + line;
        }

        // Check if we have a complete INSERT statement (ends with ");")
        if (in_insert && current_statement.find(");") != std::string::npos) {
            // Parse column names (only on first INSERT)
            if (column_names.empty()) {
                column_names = parse_column_names(current_statement);
                std::cout << "Found " << column_names.size() << " columns\n";
                std::cout << "Columns: ";
                for (size_t i = 0; i < std::min(size_t(5), column_names.size()); ++i) {
                    std::cout << column_names[i];
                    if (i < 4 && i < column_names.size() - 1) std::cout << ", ";
                }
                if (column_names.size() > 5) {
                    std::cout << ", ... (+" << (column_names.size() - 5) << " more)";
                }
                std::cout << "\n\n";
            }

            // Parse values
            auto values = parse_insert_values(current_statement);

            if (values.size() != column_names.size()) {
                std::cerr << "Warning: Record " << (record_count + 1)
                         << " - Column count mismatch (expected " << column_names.size()
                         << ", got " << values.size() << ")\n";
                in_insert = false;
                current_statement.clear();
                continue;
            }

            // Extract the work request ID from first column
            std::string id_str = extract_string(values[0]);
            if (id_str.empty()) {
                std::cerr << "Warning: Record " << (record_count + 1) << " - Empty ID, skipping\n";
                in_insert = false;
                current_statement.clear();
                continue;
            }

            int64_t work_request_id = std::stoll(id_str);
            entity_id++;
            types::EntityId entity = create_entity_id(entity_id);

            // Import each column as an atom
            for (size_t i = 0; i < column_names.size(); ++i) {
                std::string col_name = column_names[i];
                std::string value = extract_string(values[i]);

                // Store as canonical atom (deduplicated) - empty strings are stored for NULL values
                std::string tag = "workrequest." + col_name;
                std::transform(tag.begin(), tag.end(), tag.begin(), ::tolower);
                log.append(entity, tag, value, types::AtomType::Canonical);
            }

            if(record_count< 4){

                core::ProjectionEngine projector(log);
                core::Node node = projector.rebuild(entity);
                std::unordered_map<std::string, types::AtomValue> node_result;
                node_result=node.get_all();

                for (const auto& [tag, value] : node_result) {
                    std::cout << "  - " << tag;
                    if (std::holds_alternative<std::string>(value)) {
                        std::cout << " = '" << std::get<std::string>(value) << "'";
                    } else if (std::holds_alternative<int64_t>(value)) {
                        std::cout << " = " << std::get<int64_t>(value);
                    }
                    std::cout << "\n";
                }
                std::cout << "----\n";
            }
            record_count++;

            // Progress indicator
            if (record_count % 100 == 0) {
                std::cout << "Imported " << record_count << " work requests...\r" << std::flush;
            }

            // Reset for next INSERT
            in_insert = false;
            current_statement.clear();
        }
    }

    infile.close();

    std::cout << "\n\n=== Import Complete ===\n";
    std::cout << "Total work requests imported: " << record_count << "\n\n";

    // Show statistics
    auto stats = log.get_stats();
    std::cout << "=== Atom Log Statistics ===\n";
    std::cout << "Total atoms: " << stats.total_atoms << "\n";
    std::cout << "Canonical atoms: " << stats.canonical_atoms << "\n";
    std::cout << "Unique canonical atoms: " << stats.unique_canonical_atoms << "\n";
    std::cout << "Deduplicated hits: " << stats.deduplicated_hits << "\n";
    std::cout << "Deduplication rate: "
              << (stats.canonical_atoms > 0
                  ? (100.0 * stats.deduplicated_hits / stats.canonical_atoms)
                  : 0.0)
              << "%\n\n";

    // Save to disk
    std::string output_file = "workrequest_import.dat";
    std::cout << "Saving to '" << output_file << "'...\n";
    if (log.save(output_file)) {
        std::cout << "  ✓ Successfully saved\n\n";
    } else {
        std::cout << "  ✗ Failed to save\n";
        return 1;
    }

    // Demo: Query some work requests
    std::cout << "=== Query Examples ===\n";

    // Get first work request ID
    if (record_count > 0) {
        // Rebuild projection for first few entities
        core::ProjectionEngine projector(log);
        std::unordered_map<types::EntityId, gtaf::core::Node, gtaf::core::EntityIdHash> nodes;
        nodes = projector.rebuild_all();

        std::cout << "Querying first 3 work requests...\n\n";
        size_t scanned = 0;
        for(const auto& [entity_id, node] : nodes) {
            if(scanned>3) break;
            scanned++;
            std::unordered_map<std::string, types::AtomValue> node_result;
            node_result=node.get_all();

            for (const auto& [tag, value] : node_result) {
                std::cout << "  - " << tag;
                if (std::holds_alternative<std::string>(value)) {
                    std::cout << " = '" << std::get<std::string>(value) << "'";
                } else if (std::holds_alternative<int64_t>(value)) {
                    std::cout << " = " << std::get<int64_t>(value);
                }
                std::cout << "\n";
            }
            std::cout << "----\n";
            // Note: We'd need to track the actual IDs. For demo, we'll skip this.
        }

        std::vector<types::EntityId> entity_ids = projector.get_all_entities();

        scanned=0;
        for (const auto& entity_id : entity_ids) {
            if(scanned>3) break;
            scanned++;
            core::Node node = projector.rebuild(entity_id);
            std::unordered_map<std::string, types::AtomValue> node_result;
            node_result=node.get_all();

            for (const auto& [tag, value] : node_result) {
                std::cout << "  - " << tag;
                if (std::holds_alternative<std::string>(value)) {
                    std::cout << " = '" << std::get<std::string>(value) << "'";
                } else if (std::holds_alternative<int64_t>(value)) {
                    std::cout << " = " << std::get<int64_t>(value);
                }
                std::cout << "\n";
            }
            std::cout << "----\n";
        }

        std::cout << "Example: To query a specific work request:\n";
        std::cout << "  1. Create EntityId from ID\n";
        std::cout << "  2. Call projector.rebuild(entity)\n";
        std::cout << "  3. Use node.get(\"workrequest.name\") etc.\n\n";
    }

    std::cout << "=== Next Steps ===\n";
    std::cout << "1. Load saved data: log.load(\"workrequest_import.dat\")\n";
    std::cout << "2. Build projections: projector.rebuild(entity_id)\n";
    std::cout << "3. Query properties: node.get(\"workrequest.field\")\n";
    std::cout << "4. Analyze deduplication savings\n\n";

    std::cout << "=== Demo Complete ===\n";
    return 0;
}
