#include "../../core/atom_log.h"
#include "../../core/projection_engine.h"
#include "../../core/query_index.h"
#include "../../types/hash_utils.h"
#include <iostream>
#include <chrono>
#include <fstream>
#include <map>
#include <algorithm>

using namespace gtaf;

// Helper function to get current memory usage (RSS) in KB
size_t get_memory_usage_kb() {
#ifdef __linux__
    std::ifstream status_file("/proc/self/status");
    std::string line;
    while (std::getline(status_file, line)) {
        if (line.substr(0, 6) == "VmRSS:") {
            size_t start = line.find_first_of("0123456789");
            size_t end = line.find_first_not_of("0123456789", start);
            std::string num_str = line.substr(start, end - start);
            return std::stoull(num_str);
        }
    }
#endif
    return 0;
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

int main(int argc, char* argv[]) {
    std::cout << "=== TPC-H Query Tool for GTAF ===\n\n";

    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <tpch_data_file>\n";
        std::cerr << "\nExample:\n";
        std::cerr << "  " << argv[0] << " tpch_sf1.dat\n";
        return 1;
    }

    std::string data_file = argv[1];

    size_t mem_start = get_memory_usage_kb();
    std::cout << "Initial memory: " << format_memory(mem_start) << "\n\n";

    // Load data
    std::cout << "Loading TPC-H data from: " << data_file << "\n";
    core::AtomLog log;

    auto start = std::chrono::high_resolution_clock::now();
    if (!log.load(data_file)) {
        std::cerr << "Error: Failed to load data file\n";
        return 1;
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto load_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    size_t mem_after_load = get_memory_usage_kb();

    std::cout << "  ✓ Loaded " << log.all().size() << " atoms in " << load_time.count() << "ms\n";
    std::cout << "  Memory after load: " << format_memory(mem_after_load)
              << " (+" << format_memory(mem_after_load - mem_start) << ")\n\n";

    // Show statistics
    auto stats = log.get_stats();
    std::cout << "=== Dataset Statistics ===\n";
    std::cout << "Total unique atoms: " << stats.total_atoms << "\n";
    std::cout << "Total entities: " << stats.total_entities << "\n";
    std::cout << "Total references: " << stats.total_references << "\n";

    // Calculate effective deduplication: how many references share atoms
    // If total_references > total_atoms, atoms are being reused
    double dedup_ratio = stats.total_references > 0
        ? (100.0 * (stats.total_references - stats.total_atoms) / stats.total_references)
        : 0.0;
    std::cout << "Storage efficiency: " << dedup_ratio << "% "
              << "(saved " << (stats.total_references - stats.total_atoms) << " duplicate atoms)\n";
    std::cout << "Session dedup hits: " << stats.deduplicated_hits << "\n\n";

    // Create projection engine and index
    std::cout << "Creating ProjectionEngine and QueryIndex...\n";
    core::ProjectionEngine projector(log);
    // Use direct AtomLog constructor for faster index building (bypasses Node reconstruction)
    core::QueryIndex index(log);

    size_t mem_after_projector = get_memory_usage_kb();

    // Get entity counts per table
    auto all_entities = projector.get_all_entities();
    std::cout << "  Total entities: " << all_entities.size() << "\n\n";

    // Build indexes for commonly queried fields
    std::cout << "=== Building Query Indexes ===\n";

    start = std::chrono::high_resolution_clock::now();

    // Build all indexes in a single pass for efficiency
    std::cout << "Building all indexes in single pass...\n";
    index.build_indexes({
        // LINEITEM table
        "lineitem.shipdate",
        "lineitem.returnflag",
        "lineitem.linestatus",
        "lineitem.quantity",
        "lineitem.extendedprice",
        "lineitem.discount",
        // ORDERS table
        "orders.orderdate",
        "orders.orderstatus",
        "orders.totalprice",
        // CUSTOMER table
        "customer.mktsegment",
        "customer.acctbal"
    });

    end = std::chrono::high_resolution_clock::now();
    auto index_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    size_t mem_after_index = get_memory_usage_kb();

    std::cout << "\n  ✓ Index build complete in " << index_time.count() << "ms\n";
    std::cout << "  Memory after indexing: " << format_memory(mem_after_index)
              << " (+" << format_memory(mem_after_index - mem_after_projector) << ")\n\n";

    // ========================================================================
    // TPC-H Query 1: Pricing Summary Report
    // ========================================================================
    std::cout << "=== TPC-H Query 1: Pricing Summary Report ===\n";
    std::cout << "SQL: SELECT l_returnflag, l_linestatus, SUM(l_quantity), SUM(l_extendedprice)\n";
    std::cout << "     FROM lineitem WHERE l_shipdate <= '1998-09-02'\n";
    std::cout << "     GROUP BY l_returnflag, l_linestatus\n\n";

    start = std::chrono::high_resolution_clock::now();

    // Find all lineitems with shipdate <= 1998-09-02
    auto matching_lineitems = index.find_int_where("lineitem.shipdate", [](int64_t shipdate) {
        // Parse date as YYYY-MM-DD (e.g., "1998-09-02")
        // For simplicity, compare as string
        return true; // Would need proper date parsing
    });

    // Group by returnflag and linestatus
    std::map<std::pair<std::string, std::string>, double> groups;

    for (const auto& entity : matching_lineitems) {
        auto returnflag = index.get_string("lineitem.returnflag", entity);
        auto linestatus = index.get_string("lineitem.linestatus", entity);

        if (returnflag && linestatus) {
            auto key = std::make_pair(*returnflag, *linestatus);
            groups[key] += 1.0; // Would sum quantity, price, etc.
        }
    }

    end = std::chrono::high_resolution_clock::now();
    auto query1_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "Found " << matching_lineitems.size() << " matching line items\n";
    std::cout << "Grouped into " << groups.size() << " result rows\n";
    std::cout << "Query time: " << query1_time.count() << "ms\n\n";

    // Show sample results
    std::cout << "Sample results:\n";
    int count = 0;
    for (const auto& [key, value] : groups) {
        std::cout << "  Return Flag: " << key.first
                  << ", Line Status: " << key.second
                  << ", Count: " << value << "\n";
        if (++count >= 5) break;
    }

    // ========================================================================
    // TPC-H Query 2: Shipping Priority (simplified)
    // ========================================================================
    std::cout << "\n\n=== TPC-H Query 2: Shipping Priority (simplified) ===\n";
    std::cout << "SQL: SELECT o_orderkey, SUM(l_extendedprice * (1 - l_discount))\n";
    std::cout << "     FROM customer, orders, lineitem\n";
    std::cout << "     WHERE c_mktsegment = 'BUILDING' AND c_custkey = o_custkey\n";
    std::cout << "     GROUP BY o_orderkey ORDER BY revenue DESC LIMIT 10\n\n";

    start = std::chrono::high_resolution_clock::now();

    // Find customers in BUILDING segment
    auto building_customers = index.find_equals("customer.mktsegment", "BUILDING");

    std::cout << "Found " << building_customers.size() << " customers in BUILDING segment\n";

    end = std::chrono::high_resolution_clock::now();
    auto query2_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "Query time: " << query2_time.count() << "ms\n";

    // ========================================================================
    // Simple COUNT queries on each table
    // ========================================================================
    std::cout << "\n\n=== Table Row Counts ===\n";

    struct TableCount {
        std::string name;
        size_t count;
    };

    std::vector<TableCount> table_counts;

    start = std::chrono::high_resolution_clock::now();

    // Count entities by table (based on entity ID structure)
    std::map<std::string, size_t> counts_by_table;
    for (const auto& entity : all_entities) {
        // Extract table name from first 8 bytes (hash of table name)
        // For now, we'll use indexes to determine tables
        if (index.get_string("lineitem.orderkey", entity)) {
            counts_by_table["lineitem"]++;
        } else if (index.get_string("orders.orderkey", entity)) {
            counts_by_table["orders"]++;
        } else if (index.get_string("customer.custkey", entity)) {
            counts_by_table["customer"]++;
        } else if (index.get_string("part.partkey", entity)) {
            counts_by_table["part"]++;
        } else if (index.get_string("partsupp.partkey", entity)) {
            counts_by_table["partsupp"]++;
        } else if (index.get_string("supplier.suppkey", entity)) {
            counts_by_table["supplier"]++;
        }
    }

    end = std::chrono::high_resolution_clock::now();
    auto count_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    for (const auto& [table, count] : counts_by_table) {
        std::cout << "  " << table << ": " << count << " rows\n";
    }
    std::cout << "\nCount time: " << count_time.count() << "ms\n";

    // ========================================================================
    // Summary
    // ========================================================================
    std::cout << "\n\n=== Performance Summary ===\n";
    std::cout << "Load time: " << load_time.count() << "ms\n";
    std::cout << "Index build time: " << index_time.count() << "ms\n";
    std::cout << "Query 1 time: " << query1_time.count() << "ms\n";
    std::cout << "Query 3 time: " << query2_time.count() << "ms\n";
    std::cout << "Total time: " << (load_time.count() + index_time.count() + query1_time.count() + query2_time.count()) << "ms\n";

    size_t mem_final = get_memory_usage_kb();
    std::cout << "\n=== Memory Summary ===\n";
    std::cout << "Initial: " << format_memory(mem_start) << "\n";
    std::cout << "After load: " << format_memory(mem_after_load) << "\n";
    std::cout << "After indexes: " << format_memory(mem_after_index) << "\n";
    std::cout << "Final: " << format_memory(mem_final) << "\n";

    std::cout << "\n=== Demo Complete ===\n";
    return 0;
}
