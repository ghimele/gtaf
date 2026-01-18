#include "../core/atom_store.h"
#include "../core/projection_engine.h"
#include "../core/query_index.h"
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
    std::cout << "=== GTAF Query Demo with Indexes - WORKREQUEST Data ===\n\n";

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

    // Load the atom store
    core::AtomStore store;
    auto start = std::chrono::high_resolution_clock::now();

    if (!store.load(data_file)) {
        std::cerr << "Error: Failed to load data file: " << data_file << "\n";
        return 1;
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    size_t mem_after_load = get_memory_usage_kb();
    size_t mem_delta_load = mem_after_load - mem_start;

    std::cout << "  ✓ Loaded " << store.all().size() << " atoms in " << duration.count() << "ms\n";
    std::cout << "  Memory after load: " << format_memory(mem_after_load)
              << " (+" << format_memory(mem_delta_load) << ")\n\n";

    // Create projection engine and index
    std::cout << "Creating ProjectionEngine and QueryIndex...\n";
    core::ProjectionEngine projector(store);
    core::QueryIndex index(projector);

    size_t mem_after_projector = get_memory_usage_kb();
    std::cout << "  Memory after ProjectionEngine: " << format_memory(mem_after_projector) << "\n\n";

    // Build indexes for commonly queried fields
    std::cout << "=== Building Query Indexes ===\n";
    std::cout << "This builds lightweight indexes (only the queried fields, not full nodes)\n\n";

    start = std::chrono::high_resolution_clock::now();

    // Build all indexes in a single pass for efficiency
    std::cout << "Building all indexes in single pass...\n";
    size_t total_indexed = index.build_indexes({
        "workrequest.description",
        "workrequest.attacheddesignid",
        "workrequest.workrequeststateid",
        "workrequest.name",
        "workrequest.customername",
        "workrequest.cstemaxstatus"
    });
    std::cout << "  ✓ Total indexed entries: " << total_indexed << "\n";

    end = std::chrono::high_resolution_clock::now();
    auto index_build_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    size_t mem_after_index = get_memory_usage_kb();
    size_t mem_delta_index = mem_after_index - mem_after_projector;

    std::cout << "\nIndex build complete in " << index_build_time.count() << "ms\n";
    std::cout << "Memory after indexing: " << format_memory(mem_after_index)
              << " (+" << format_memory(mem_delta_index) << ")\n\n";

    auto index_stats = index.get_stats();
    std::cout << "Index statistics:\n";
    std::cout << "  - Indexed tags: " << index_stats.num_indexed_tags << "\n";
    std::cout << "  - Total entries: " << index_stats.total_entries << "\n\n";

    // ========================================================================
    // QUERY 1: Select all work requests with description LIKE '%ADDS%'
    // ========================================================================
    std::cout << "=== QUERY 1: Description LIKE '%ADDS%' ===\n";

    start = std::chrono::high_resolution_clock::now();

    // Use index to find matching entities (very fast)
    auto matching_entities1 = index.find_contains("workrequest.description", "ADDS");

    end = std::chrono::high_resolution_clock::now();
    auto query_duration1 = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    size_t mem_after_query1 = get_memory_usage_kb();

    std::cout << "Found " << matching_entities1.size() << " results in " << query_duration1.count() << "ms\n";
    std::cout << "Memory after Query 1: " << format_memory(mem_after_query1) << "\n";

    // Show first 5 results
    for (size_t i = 0; i < std::min(size_t(5), matching_entities1.size()); ++i) {
        const auto& entity = matching_entities1[i];
        auto desc = index.get_string("workrequest.description", entity);
        auto name = index.get_string("workrequest.name", entity);

        std::cout << "\n[" << (i + 1) << "] Work Request ID: " << extract_entity_id(entity) << "\n";
        if (name) std::cout << "    Name: " << *name << "\n";
        if (desc) {
            std::string desc_str = *desc;
            std::cout << "    Description: " << desc_str.substr(0, 80);
            if (desc_str.length() > 80) std::cout << "...";
            std::cout << "\n";
        }
    }

    if (matching_entities1.size() > 5) {
        std::cout << "\n... and " << (matching_entities1.size() - 5) << " more results\n";
    }

    // ========================================================================
    // QUERY 2: Select all work requests with ATTACHEDDESIGNID > 0
    // ========================================================================
    std::cout << "\n\n=== QUERY 2: ATTACHEDDESIGNID > 0 ===\n";

    start = std::chrono::high_resolution_clock::now();

    // Use index with predicate
    auto matching_entities2 = index.find_int_where("workrequest.attacheddesignid", [](int64_t val) {
        return val > 0;
    });

    end = std::chrono::high_resolution_clock::now();
    auto query_duration2 = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    size_t mem_after_query2 = get_memory_usage_kb();

    std::cout << "Found " << matching_entities2.size() << " results in " << query_duration2.count() << "ms\n";
    std::cout << "Memory after Query 2: " << format_memory(mem_after_query2) << "\n";

    // Show first 5 results
    for (size_t i = 0; i < std::min(size_t(5), matching_entities2.size()); ++i) {
        const auto& entity = matching_entities2[i];
        auto name = index.get_string("workrequest.name", entity);
        auto design_id_str = index.get_string("workrequest.attacheddesignid", entity);
        auto customer = index.get_string("workrequest.customername", entity);
        auto desc = index.get_string("workrequest.description", entity);

        std::cout << "\n[" << (i + 1) << "] Work Request ID: " << extract_entity_id(entity) << "\n";
        if (name) std::cout << "    Name: " << *name << "\n";
        if (design_id_str) std::cout << "    Design ID: " << *design_id_str << "\n";
        if (customer) std::cout << "    Customer: " << *customer << "\n";
        if (desc) {
            std::string desc_str = *desc;
            std::cout << "    Description: " << desc_str.substr(0, 80);
            if (desc_str.length() > 80) std::cout << "...";
            std::cout << "\n";
        }
    }

    if (matching_entities2.size() > 5) {
        std::cout << "\n... and " << (matching_entities2.size() - 5) << " more results\n";
    }

    // ========================================================================
    // QUERY 3: Select all work requests with WORKREQUESTSTATEID = 1
    // ========================================================================
    std::cout << "\n\n=== QUERY 3: WORKREQUESTSTATEID = 1 ===\n";

    start = std::chrono::high_resolution_clock::now();

    // Use index for exact match
    auto matching_entities3 = index.find_equals("workrequest.workrequeststateid", "1");

    end = std::chrono::high_resolution_clock::now();
    auto query_duration3 = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    size_t mem_after_query3 = get_memory_usage_kb();

    std::cout << "Found " << matching_entities3.size() << " results in " << query_duration3.count() << "ms\n";
    std::cout << "Memory after Query 3: " << format_memory(mem_after_query3) << "\n";

    // Show first 5 results
    for (size_t i = 0; i < std::min(size_t(5), matching_entities3.size()); ++i) {
        const auto& entity = matching_entities3[i];
        auto name = index.get_string("workrequest.name", entity);
        auto customer = index.get_string("workrequest.customername", entity);
        auto status = index.get_string("workrequest.cstemaxstatus", entity);
        auto desc = index.get_string("workrequest.description", entity);

        std::cout << "\n[" << (i + 1) << "] Work Request ID: " << extract_entity_id(entity) << "\n";
        if (name) std::cout << "    Name: " << *name << "\n";
        std::cout << "    State ID: 1\n";
        if (status) std::cout << "    Status: " << *status << "\n";
        if (customer) std::cout << "    Customer: " << *customer << "\n";
        if (desc) {
            std::string desc_str = *desc;
            std::cout << "    Description: " << desc_str.substr(0, 80);
            if (desc_str.length() > 80) std::cout << "...";
            std::cout << "\n";
        }
    }

    if (matching_entities3.size() > 5) {
        std::cout << "\n... and " << (matching_entities3.size() - 5) << " more results\n";
    }

    // ========================================================================
    // Summary
    // ========================================================================
    std::cout << "\n\n=== Query Summary ===\n";
    std::cout << "Index build time: " << index_build_time.count() << "ms (done once, reused for all queries)\n";
    std::cout << "Query 1 (Description LIKE '%ADDS%'): " << matching_entities1.size() << " results in " << query_duration1.count() << "ms\n";
    std::cout << "Query 2 (ATTACHEDDESIGNID > 0): " << matching_entities2.size() << " results in " << query_duration2.count() << "ms\n";
    std::cout << "Query 3 (WORKREQUESTSTATEID = 1): " << matching_entities3.size() << " results in " << query_duration3.count() << "ms\n";
    std::cout << "Total query time: " << (query_duration1.count() + query_duration2.count() + query_duration3.count()) << "ms\n";
    std::cout << "Total time (index + queries): " << (index_build_time.count() + query_duration1.count() + query_duration2.count() + query_duration3.count()) << "ms\n";

    std::cout << "\n=== Performance Notes ===\n";
    std::cout << "• Indexes built ONCE for commonly queried fields\n";
    std::cout << "• Queries use indexes for instant filtering (no node scanning)\n";
    std::cout << "• Memory usage is LOW (only indexed fields stored, not full nodes)\n";
    std::cout << "• Query time is VERY FAST (O(n) scan of index, not O(n) node rebuilds)\n";
    std::cout << "• Best for read-heavy workloads with repetitive query patterns\n";

    // Final memory summary
    size_t mem_final = get_memory_usage_kb();
    std::cout << "\n=== Memory Summary ===\n";
    std::cout << "Initial:              " << format_memory(mem_start) << "\n";
    std::cout << "After load:           " << format_memory(mem_after_load)
              << " (+" << format_memory(mem_delta_load) << ")\n";
    std::cout << "After ProjectionEngine: " << format_memory(mem_after_projector) << "\n";
    std::cout << "After indexes:        " << format_memory(mem_after_index)
              << " (+" << format_memory(mem_delta_index) << ")\n";
    std::cout << "After Query 1:        " << format_memory(mem_after_query1) << "\n";
    std::cout << "After Query 2:        " << format_memory(mem_after_query2) << "\n";
    std::cout << "After Query 3:        " << format_memory(mem_after_query3) << "\n";
    std::cout << "Final:                " << format_memory(mem_final)
              << " (Total: +" << format_memory(mem_final - mem_start) << ")\n";

    std::cout << "\n=== Demo Complete ===\n";
    return 0;
}
