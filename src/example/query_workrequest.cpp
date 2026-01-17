#include "../core/atom_log.h"
#include "../core/projection_engine.h"
#include "../types/hash_utils.h"
#include "../core/node.h"

#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <algorithm>
#include <unordered_set>
#include <fstream>
#include <cstdlib>

using namespace gtaf;

// Helper function to get current memory usage (RSS) in KB
size_t get_memory_usage_kb() {
#ifdef __linux__
    std::ifstream status_file("/proc/self/status");
    std::string line;
    while (std::getline(status_file, line)) {
        if (line.substr(0, 6) == "VmRSS:") {
            // Extract the number from "VmRSS: 12345 kB"
            size_t start = line.find_first_of("0123456789");
            size_t end = line.find_first_not_of("0123456789", start);
            std::string num_str = line.substr(start, end - start);
            return std::stoull(num_str);
        }
    }
#endif
    return 0; // Not supported on non-Linux systems
}

// Helper to format memory size
std::string format_memory(size_t kb) {
    if (kb >= 1024 * 1024) {
        return std::to_string(kb / (1024 * 1024)) + " GB (" + std::to_string(kb) + " KB)";
    } else if (kb >= 1024) {
        return std::to_string(kb / 1024) + " MB (" + std::to_string(kb) + " KB)";
    } else {
        return std::to_string(kb) + " KB";
    }
}

// Helper to create EntityId from integer ID
types::EntityId create_entity_id(int64_t id) {
    types::EntityId entity{};
    std::fill(entity.bytes.begin(), entity.bytes.end(), 0);

    // Store the ID in the first 8 bytes (little-endian)
    for (int i = 0; i < 8; ++i) {
        entity.bytes[i] = static_cast<uint8_t>((id >> (i * 8)) & 0xFF);
    }

    return entity;
}

// Helper to extract ID from EntityId
int64_t extract_entity_id(const types::EntityId& entity) {
    int64_t id = 0;
    for (int i = 0; i < 8; ++i) {
        id |= (static_cast<int64_t>(entity.bytes[i]) << (i * 8));
    }
    return id;
}

// Query result structure
struct WorkRequest {
    int64_t id;
    std::string name;
    std::string description;
    std::string customer_name;
    int64_t state_id;
    int64_t design_id;
    std::string work_type;
    std::string status;
};

int main(int argc, char* argv[]) {
    std::cout << "=== GTAF Query Demo - WORKREQUEST Data ===\n\n";

    // Memory tracking
    size_t mem_start = get_memory_usage_kb();
    std::cout << "=== Memory Usage ===\n";
    std::cout << "Initial memory: " << format_memory(mem_start) << "\n\n";

    // Determine input file
    std::string data_file = "../test_data/workrequest_import.dat";
    if (argc > 1) {
        data_file = argv[1];
    }

    std::cout << "Loading data from: " << data_file << "\n";

    // Load the atom log
    core::AtomLog log;
    auto start = std::chrono::high_resolution_clock::now();

    if (!log.load(data_file)) {
        std::cerr << "Error: Failed to load data file: " << data_file << "\n";
        return 1;
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    size_t mem_after_load = get_memory_usage_kb();
    size_t mem_delta_load = mem_after_load - mem_start;

    std::cout << "  ✓ Loaded " << log.all().size() << " atoms in " << duration.count() << "ms\n";
    std::cout << "  Memory after load: " << format_memory(mem_after_load)
              << " (+" << format_memory(mem_delta_load) << ")\n\n";

    // Show statistics
    auto stats = log.get_stats();
    std::cout << "=== Atom Log Statistics ===\n";
    std::cout << "Total atoms: " << stats.total_atoms << "\n";
    std::cout << "Canonical atoms: " << stats.canonical_atoms << "\n";
    std::cout << "Unique canonical: " << stats.unique_canonical_atoms << "\n";
    std::cout << "Deduplication rate: "
              << (stats.canonical_atoms > 0
                  ? (100.0 * stats.deduplicated_hits / stats.canonical_atoms)
                  : 0.0)
              << "%\n\n";

    // Create projection engine
    std::cout << "Creating ProjectionEngine...\n";
    core::ProjectionEngine projector(log);

    size_t mem_after_projector = get_memory_usage_kb();
    size_t mem_delta_projector = mem_after_projector - mem_after_load;
    std::cout << "  Memory after ProjectionEngine: " << format_memory(mem_after_projector)
              << " (+" << format_memory(mem_delta_projector) << ")\n\n";

    // Get all entity IDs efficiently using ProjectionEngine
    std::cout << "=== Scanning for Work Request Entities ===\n";
    std::vector<types::EntityId> entity_ids = projector.get_all_entities();
    size_t total_entities = entity_ids.size();

    std::cout << "Found " << total_entities << " unique work request entities\n";
    std::cout << "Building all projections once (will reuse for all queries)...\n";

    // Build all nodes ONCE and keep in memory for all queries
    start = std::chrono::high_resolution_clock::now();
    std::unordered_map<types::EntityId, gtaf::core::Node, gtaf::core::EntityIdHash> nodes;
    nodes = projector.rebuild_all();
    end = std::chrono::high_resolution_clock::now();
    auto build_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    size_t mem_after_rebuild = get_memory_usage_kb();
    size_t mem_delta_rebuild = mem_after_rebuild - mem_after_projector;

    std::cout << "  ✓ Built " << nodes.size() << " projections in " << build_duration.count() << "ms\n";
    std::cout << "  Memory after rebuild: " << format_memory(mem_after_rebuild)
              << " (+" << format_memory(mem_delta_rebuild) << ")\n\n";

    // ========================================================================
    // QUERY 1: Select all work requests with description LIKE '%ADDS%'
    // ========================================================================
    std::cout << "=== QUERY 1: Description LIKE '%ADDS%' ===\n";

    start = std::chrono::high_resolution_clock::now();

    // Query the pre-built nodes
    size_t scanned = 0;
    size_t match_count = 0;
    std::vector<std::unordered_map<std::string, types::AtomValue>> first_5_results;

    for (const auto& [entity_id, node] : nodes) {
        scanned++;

        auto desc = node.get("workrequest.description");

        if (desc && std::holds_alternative<std::string>(*desc)) {
            std::string description = std::get<std::string>(*desc);

            // Case-insensitive search for 'ADDS'
            std::string upper_desc = description;
            std::transform(upper_desc.begin(), upper_desc.end(), upper_desc.begin(), ::toupper);

            if (upper_desc.find("ADDS") != std::string::npos) {
                match_count++;

                // Only keep first 5 results in memory for display
                if (first_5_results.size() < 5) {
                    first_5_results.push_back(node.get_all());
                }
            }
        }

        // Progress indicator for large datasets
        if (total_entities > 1000 && scanned % 1000 == 0) {
            std::cout << "  Scanned " << scanned << "/" << total_entities << " entities...\r" << std::flush;
        }
    }

    end = std::chrono::high_resolution_clock::now();
    auto query_duration1 = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    size_t mem_after_query1 = get_memory_usage_kb();
    size_t mem_delta_query1 = mem_after_query1 - mem_after_rebuild;

    std::flush(std::cout);
    std::cout << "Found " << match_count << " results in " << query_duration1.count() << "ms\n";
    std::cout << "Memory after Query 1: " << format_memory(mem_after_query1)
              << " (+" << format_memory(mem_delta_query1) << ")\n";

    // Show first 5 results
    for (size_t i = 0; i < first_5_results.size(); ++i) {
        const auto& node_result = first_5_results[i];

        for (const auto& [tag, value] : node_result) {
            std::cout << "  - " << tag;
            if (std::holds_alternative<std::string>(value)) {
                std::cout << " = '" << std::get<std::string>(value) << "'";
            } else if (std::holds_alternative<int64_t>(value)) {
                std::cout << " = " << std::get<int64_t>(value);
            }
            std::cout << "\n";
        }
        std::cout << "---\n";
    }

    if (match_count > 5) {
        std::cout << "\n... and " << (match_count - 5) << " more results\n";
    }

    // ========================================================================
    // QUERY 2: Select all work requests with ATTACHEDDESIGNID > 0
    // ========================================================================
    std::cout << "\n\n=== QUERY 2: ATTACHEDDESIGNID > 0 ===\n";

    std::vector<WorkRequest> first_5_results2;
    start = std::chrono::high_resolution_clock::now();

    scanned = 0;
    match_count = 0;
    for (const auto& [entity_id, node] : nodes) {
        scanned++;

        auto design_id_val = node.get("workrequest.attacheddesignid");

        if (design_id_val && std::holds_alternative<std::string>(*design_id_val)) {
            std::string design_id_str = std::get<std::string>(*design_id_val);

            // Try to parse as integer
            if (!design_id_str.empty()) {
                try {
                    int64_t design_id = std::stoll(design_id_str);

                    if (design_id > 0) {
                        match_count++;

                        // Only keep first 5 results in memory
                        if (first_5_results2.size() < 5) {
                            WorkRequest wr;
                            wr.id = extract_entity_id(entity_id);
                            wr.design_id = design_id;

                            // Get other fields
                            if (auto name = node.get("workrequest.name")) {
                                if (std::holds_alternative<std::string>(*name)) {
                                    wr.name = std::get<std::string>(*name);
                                }
                            }

                            if (auto desc = node.get("workrequest.description")) {
                                if (std::holds_alternative<std::string>(*desc)) {
                                    wr.description = std::get<std::string>(*desc);
                                }
                            }

                            if (auto customer = node.get("workrequest.customername")) {
                                if (std::holds_alternative<std::string>(*customer)) {
                                    wr.customer_name = std::get<std::string>(*customer);
                                }
                            }

                            first_5_results2.push_back(wr);
                        }
                    }
                } catch (...) {
                    // Skip invalid integers
                }
            }
        }

        // Progress indicator for large datasets
        if (total_entities > 1000 && scanned % 1000 == 0) {
            std::cout << "  Scanned " << scanned << "/" << total_entities << " entities...\r" << std::flush;
        }
    }

    end = std::chrono::high_resolution_clock::now();
    auto query_duration2 = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    size_t mem_after_query2 = get_memory_usage_kb();
    size_t mem_delta_query2 = mem_after_query2 - mem_after_query1;

    std::flush(std::cout);
    std::cout << "Found " << match_count << " results in " << query_duration2.count() << "ms\n";
    std::cout << "Memory after Query 2: " << format_memory(mem_after_query2)
              << " (+" << format_memory(mem_delta_query2) << ")\n";

    // Show first 5 results
    for (size_t i = 0; i < first_5_results2.size(); ++i) {
        const auto& wr = first_5_results2[i];
        std::cout << "\n[" << (i + 1) << "] Work Request ID: " << wr.id << "\n";
        std::cout << "    Name: " << wr.name << "\n";
        std::cout << "    Design ID: " << wr.design_id << "\n";
        std::cout << "    Customer: " << wr.customer_name << "\n";
        std::cout << "    Description: " << wr.description.substr(0, 80);
        if (wr.description.length() > 80) {
            std::cout << "...";
        }
        std::cout << "\n";
    }

    if (match_count > 5) {
        std::cout << "\n... and " << (match_count - 5) << " more results\n";
    }

    // ========================================================================
    // QUERY 3: Select all work requests with WORKREQUESTSTATEID = 1
    // ========================================================================
    std::cout << "\n\n=== QUERY 3: WORKREQUESTSTATEID = 1 ===\n";

    std::vector<WorkRequest> first_5_results3;
    start = std::chrono::high_resolution_clock::now();

    scanned = 0;
    match_count = 0;
    for (const auto& [entity_id, node] : nodes) {
        scanned++;

        auto state_id_val = node.get("workrequest.workrequeststateid");

        if (state_id_val && std::holds_alternative<std::string>(*state_id_val)) {
            std::string state_id_str = std::get<std::string>(*state_id_val);

            if (state_id_str == "1") {
                match_count++;

                // Only keep first 5 results in memory
                if (first_5_results3.size() < 5) {
                    WorkRequest wr;
                    wr.id = extract_entity_id(entity_id);
                    wr.state_id = 1;

                    // Get other fields
                    if (auto name = node.get("workrequest.name")) {
                        if (std::holds_alternative<std::string>(*name)) {
                            wr.name = std::get<std::string>(*name);
                        }
                    }

                    if (auto desc = node.get("workrequest.description")) {
                        if (std::holds_alternative<std::string>(*desc)) {
                            wr.description = std::get<std::string>(*desc);
                        }
                    }

                    if (auto customer = node.get("workrequest.customername")) {
                        if (std::holds_alternative<std::string>(*customer)) {
                            wr.customer_name = std::get<std::string>(*customer);
                        }
                    }

                    if (auto status = node.get("workrequest.cstemaxstatus")) {
                        if (std::holds_alternative<std::string>(*status)) {
                            wr.status = std::get<std::string>(*status);
                        }
                    }

                    first_5_results3.push_back(wr);
                }
            }
        }

        // Progress indicator for large datasets
        if (total_entities > 1000 && scanned % 1000 == 0) {
            std::cout << "  Scanned " << scanned << "/" << total_entities << " entities...\r" << std::flush;
        }
    }

    end = std::chrono::high_resolution_clock::now();
    auto query_duration3 = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    size_t mem_after_query3 = get_memory_usage_kb();
    size_t mem_delta_query3 = mem_after_query3 - mem_after_query2;

    std::cout << "Found " << match_count << " results in " << query_duration3.count() << "ms\n";
    std::cout << "Memory after Query 3: " << format_memory(mem_after_query3)
              << " (+" << format_memory(mem_delta_query3) << ")\n";

    // Show first 5 results
    for (size_t i = 0; i < first_5_results3.size(); ++i) {
        const auto& wr = first_5_results3[i];
        std::cout << "\n[" << (i + 1) << "] Work Request ID: " << wr.id << "\n";
        std::cout << "    Name: " << wr.name << "\n";
        std::cout << "    State ID: " << wr.state_id << "\n";
        std::cout << "    Status: " << wr.status << "\n";
        std::cout << "    Customer: " << wr.customer_name << "\n";
        std::cout << "    Description: " << wr.description.substr(0, 80);
        if (wr.description.length() > 80) {
            std::cout << "...";
        }
        std::cout << "\n";
    }

    if (match_count > 5) {
        std::cout << "\n... and " << (match_count - 5) << " more results\n";
    }

    // ========================================================================
    // Clean up: Free node memory after all queries complete
    // ========================================================================
    std::cout << "\n\nFreeing projection memory...\n";
    nodes.clear();  // Explicitly free all nodes

    size_t mem_after_cleanup = get_memory_usage_kb();
    size_t mem_freed = mem_after_query3 - mem_after_cleanup;
    std::cout << "  ✓ Freed " << format_memory(mem_freed) << "\n";
    std::cout << "  Memory after cleanup: " << format_memory(mem_after_cleanup) << "\n";

    // ========================================================================
    // Summary
    // ========================================================================
    std::cout << "\n=== Query Summary ===\n";
    std::cout << "Rebuild time: " << build_duration.count() << "ms (done once, reused for all queries)\n";
    std::cout << "Query 1 (Description LIKE '%ADDS%'): " << first_5_results.size() << " shown in " << query_duration1.count() << "ms\n";
    std::cout << "Query 2 (ATTACHEDDESIGNID > 0): " << first_5_results2.size() << " shown in " << query_duration2.count() << "ms\n";
    std::cout << "Query 3 (WORKREQUESTSTATEID = 1): " << first_5_results3.size() << " shown in " << query_duration3.count() << "ms\n";
    std::cout << "Total query time: " << (query_duration1.count() + query_duration2.count() + query_duration3.count()) << "ms\n";
    std::cout << "Total time (rebuild + queries): " << (build_duration.count() + query_duration1.count() + query_duration2.count() + query_duration3.count()) << "ms\n";

    std::cout << "\n=== Performance Notes ===\n";
    std::cout << "• Projections built ONCE and reused for all queries (optimal for multiple queries)\n";
    std::cout << "• Only first 5 results stored in memory for display\n";
    std::cout << "• Each query scans pre-built nodes (no rebuild overhead)\n";
    std::cout << "• Property lookups are O(1) hash table operations\n";
    std::cout << "• Query times measured in milliseconds\n";

    // Final memory summary
    std::cout << "\n=== Memory Summary ===\n";
    std::cout << "Initial:              " << format_memory(mem_start) << "\n";
    std::cout << "After load:           " << format_memory(mem_after_load)
              << " (+" << format_memory(mem_delta_load) << ")\n";
    std::cout << "After ProjectionEngine: " << format_memory(mem_after_projector)
              << " (+" << format_memory(mem_delta_projector) << ")\n";
    std::cout << "After rebuild_all():  " << format_memory(mem_after_rebuild)
              << " (+" << format_memory(mem_delta_rebuild) << ") [PEAK]\n";
    std::cout << "After Query 1:        " << format_memory(mem_after_query1)
              << " (+" << format_memory(mem_delta_query1) << ")\n";
    std::cout << "After Query 2:        " << format_memory(mem_after_query2)
              << " (+" << format_memory(mem_delta_query2) << ")\n";
    std::cout << "After Query 3:        " << format_memory(mem_after_query3)
              << " (+" << format_memory(mem_delta_query3) << ")\n";
    std::cout << "After cleanup:        " << format_memory(mem_after_cleanup)
              << " (-" << format_memory(mem_freed) << ")\n";
    std::cout << "Final:                " << format_memory(mem_after_cleanup)
              << " (Net: +" << format_memory(mem_after_cleanup - mem_start) << ")\n";

    std::cout << "\n=== Demo Complete ===\n";
    return 0;
}
