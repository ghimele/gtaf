#include "../../core/atom_store.h"
#include "../../types/hash_utils.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <chrono>
#include <cstring>

using namespace gtaf;

// Batch append structure for performance
struct BatchAtom {
    types::EntityId entity;
    std::string tag;
    types::AtomValue value;
    types::AtomType classification;
};

// Batch append function
void batch_append(core::AtomStore& store, const std::vector<BatchAtom>& batch) {
    for (const auto& atom : batch) {
        store.append(atom.entity, atom.tag, atom.value, atom.classification);
    }
}

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

// Pre-computed table hashes for performance
static const std::unordered_map<std::string, uint64_t> table_hashes = {
    {"region", std::hash<std::string>{}("region")},
    {"nation", std::hash<std::string>{}("nation")},
    {"supplier", std::hash<std::string>{}("supplier")},
    {"customer", std::hash<std::string>{}("customer")},
    {"part", std::hash<std::string>{}("part")},
    {"partsupp", std::hash<std::string>{}("partsupp")},
    {"orders", std::hash<std::string>{}("orders")},
    {"lineitem", std::hash<std::string>{}("lineitem")}
};

// Helper to create EntityId from table name and key (optimized)
types::EntityId create_entity_id(const std::string& table, int64_t key) {
    types::EntityId entity{};
    std::fill(entity.bytes.begin(), entity.bytes.end(), 0);

    // Use pre-computed table hash in first 8 bytes, key in second 8 bytes
    auto it = table_hashes.find(table);
    uint64_t table_hash = (it != table_hashes.end()) ? it->second : std::hash<std::string>{}(table);

    std::memcpy(entity.bytes.data(), &table_hash, 8);
    std::memcpy(entity.bytes.data() + 8, &key, 8);

    return entity;
}

// Parse a pipe-delimited line from TPC-H .tbl file (optimized)
void parse_tbl_line(const std::string& line, std::vector<std::string>& fields) {
    fields.clear();
    
    size_t start = 0;
    size_t end = line.find('|');
    
    while (end != std::string::npos) {
        fields.emplace_back(line.substr(start, end - start));
        start = end + 1;
        end = line.find('|', start);
    }
    
    // Handle last field (if any)
    if (start < line.length()) {
        fields.emplace_back(line.substr(start));
    }
}

// Import REGION table (5 rows)
// Format: R_REGIONKEY|R_NAME|R_COMMENT|
size_t import_region(core::AtomStore& store, const std::string& filename) {
    std::cout << "Importing REGION from: " << filename << "\n";

    std::ifstream file(filename);
    file.rdbuf()->pubsetbuf(nullptr, 64 * 1024); // 64KB buffer
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open " << filename << "\n";
        return 0;
    }

    size_t row_count = 0;
    std::string line;
    std::vector<std::string> fields;
    fields.reserve(3);
    std::vector<BatchAtom> batch;
    batch.reserve(1000);

    while (std::getline(file, line)) {
        if (line.empty()) continue;

        parse_tbl_line(line, fields);
        if (fields.size() < 3) continue;

        int64_t regionkey = std::stoll(fields[0]);
        types::EntityId entity = create_entity_id("region", regionkey);

        batch.push_back({entity, "region.regionkey", fields[0], types::AtomType::Canonical});
        batch.push_back({entity, "region.name", fields[1], types::AtomType::Canonical});
        batch.push_back({entity, "region.comment", fields[2], types::AtomType::Canonical});

        row_count++;

        if (batch.size() >= 3000) {
            batch_append(store, batch);
            batch.clear();
        }
    }

    if (!batch.empty()) {
        batch_append(store, batch);
    }

    std::cout << "  ✓ Imported " << row_count << " regions\n";
    return row_count;
}

// Import NATION table (25 rows)
// Format: N_NATIONKEY|N_NAME|N_REGIONKEY|N_COMMENT|
size_t import_nation(core::AtomStore& store, const std::string& filename) {
    std::cout << "Importing NATION from: " << filename << "\n";

    std::ifstream file(filename);
    file.rdbuf()->pubsetbuf(nullptr, 64 * 1024); // 64KB buffer
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open " << filename << "\n";
        return 0;
    }

    size_t row_count = 0;
    std::string line;
    std::vector<std::string> fields;
    fields.reserve(4);
    std::vector<BatchAtom> batch;
    batch.reserve(1000);

    while (std::getline(file, line)) {
        if (line.empty()) continue;

        parse_tbl_line(line, fields);
        if (fields.size() < 4) continue;

        int64_t nationkey = std::stoll(fields[0]);
        types::EntityId entity = create_entity_id("nation", nationkey);

        batch.push_back({entity, "nation.nationkey", fields[0], types::AtomType::Canonical});
        batch.push_back({entity, "nation.name", fields[1], types::AtomType::Canonical});
        batch.push_back({entity, "nation.regionkey", fields[2], types::AtomType::Canonical});
        batch.push_back({entity, "nation.comment", fields[3], types::AtomType::Canonical});

        row_count++;

        if (batch.size() >= 4000) {
            batch_append(store, batch);
            batch.clear();
        }
    }

    if (!batch.empty()) {
        batch_append(store, batch);
    }

    std::cout << "  ✓ Imported " << row_count << " nations\n";
    return row_count;
}

// Import SUPPLIER table (10K × SF rows)
// Format: S_SUPPKEY|S_NAME|S_ADDRESS|S_NATIONKEY|S_PHONE|S_ACCTBAL|S_COMMENT|
size_t import_supplier(core::AtomStore& store, const std::string& filename) {
    std::cout << "Importing SUPPLIER from: " << filename << "\n";

    std::ifstream file(filename);
    file.rdbuf()->pubsetbuf(nullptr, 256 * 1024); // 256KB buffer for larger files
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open " << filename << "\n";
        return 0;
    }

    size_t row_count = 0;
    std::string line;
    std::vector<std::string> fields;
    fields.reserve(7);
    std::vector<BatchAtom> batch;
    batch.reserve(7000);

    while (std::getline(file, line)) {
        if (line.empty()) continue;

        parse_tbl_line(line, fields);
        if (fields.size() < 7) continue;

        int64_t suppkey = std::stoll(fields[0]);
        types::EntityId entity = create_entity_id("supplier", suppkey);

        batch.push_back({entity, "supplier.suppkey", fields[0], types::AtomType::Canonical});
        batch.push_back({entity, "supplier.name", fields[1], types::AtomType::Canonical});
        batch.push_back({entity, "supplier.address", fields[2], types::AtomType::Canonical});
        batch.push_back({entity, "supplier.nationkey", fields[3], types::AtomType::Canonical});
        batch.push_back({entity, "supplier.phone", fields[4], types::AtomType::Canonical});
        batch.push_back({entity, "supplier.acctbal", fields[5], types::AtomType::Canonical});
        batch.push_back({entity, "supplier.comment", fields[6], types::AtomType::Canonical});

        row_count++;

        if (batch.size() >= 70000) {
            batch_append(store, batch);
            batch.clear();
        }

        if (row_count % 10000 == 0) {
            std::cout << "  Processed " << row_count << " suppliers...\r" << std::flush;
        }
    }

    if (!batch.empty()) {
        batch_append(store, batch);
    }

    std::cout << "\n  ✓ Imported " << row_count << " suppliers\n";
    return row_count;
}

// Import CUSTOMER table (150K × SF rows)
// Format: C_CUSTKEY|C_NAME|C_ADDRESS|C_NATIONKEY|C_PHONE|C_ACCTBAL|C_MKTSEGMENT|C_COMMENT|
size_t import_customer(core::AtomStore& store, const std::string& filename) {
    std::cout << "Importing CUSTOMER from: " << filename << "\n";

    std::ifstream file(filename);
    file.rdbuf()->pubsetbuf(nullptr, 512 * 1024); // 512KB buffer for large files
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open " << filename << "\n";
        return 0;
    }

    size_t row_count = 0;
    std::string line;
    std::vector<std::string> fields;
    fields.reserve(8);
    std::vector<BatchAtom> batch;
    batch.reserve(8000);

    while (std::getline(file, line)) {
        if (line.empty()) continue;

        parse_tbl_line(line, fields);
        if (fields.size() < 8) continue;

        int64_t custkey = std::stoll(fields[0]);
        types::EntityId entity = create_entity_id("customer", custkey);

        batch.push_back({entity, "customer.custkey", fields[0], types::AtomType::Canonical});
        batch.push_back({entity, "customer.name", fields[1], types::AtomType::Canonical});
        batch.push_back({entity, "customer.address", fields[2], types::AtomType::Canonical});
        batch.push_back({entity, "customer.nationkey", fields[3], types::AtomType::Canonical});
        batch.push_back({entity, "customer.phone", fields[4], types::AtomType::Canonical});
        batch.push_back({entity, "customer.acctbal", fields[5], types::AtomType::Canonical});
        batch.push_back({entity, "customer.mktsegment", fields[6], types::AtomType::Canonical});
        batch.push_back({entity, "customer.comment", fields[7], types::AtomType::Canonical});

        row_count++;

        if (batch.size() >= 80000) {
            batch_append(store, batch);
            batch.clear();
        }

        if (row_count % 50000 == 0) {
            std::cout << "  Processed " << row_count << " customers...\r" << std::flush;
        }
    }

    if (!batch.empty()) {
        batch_append(store, batch);
    }

    std::cout << "\n  ✓ Imported " << row_count << " customers\n";
    return row_count;
}

// Import PART table (200K × SF rows)
// Format: P_PARTKEY|P_NAME|P_MFGR|P_BRAND|P_TYPE|P_SIZE|P_CONTAINER|P_RETAILPRICE|P_COMMENT|
size_t import_part(core::AtomStore& store, const std::string& filename) {
    std::cout << "Importing PART from: " << filename << "\n";

    std::ifstream file(filename);
    file.rdbuf()->pubsetbuf(nullptr, 512 * 1024); // 512KB buffer for large files
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open " << filename << "\n";
        return 0;
    }

    size_t row_count = 0;
    std::string line;
    std::vector<std::string> fields;
    fields.reserve(9);
    std::vector<BatchAtom> batch;
    batch.reserve(9000);

    while (std::getline(file, line)) {
        if (line.empty()) continue;

        parse_tbl_line(line, fields);
        if (fields.size() < 9) continue;

        int64_t partkey = std::stoll(fields[0]);
        types::EntityId entity = create_entity_id("part", partkey);

        batch.push_back({entity, "part.partkey", fields[0], types::AtomType::Canonical});
        batch.push_back({entity, "part.name", fields[1], types::AtomType::Canonical});
        batch.push_back({entity, "part.mfgr", fields[2], types::AtomType::Canonical});
        batch.push_back({entity, "part.brand", fields[3], types::AtomType::Canonical});
        batch.push_back({entity, "part.type", fields[4], types::AtomType::Canonical});
        batch.push_back({entity, "part.size", fields[5], types::AtomType::Canonical});
        batch.push_back({entity, "part.container", fields[6], types::AtomType::Canonical});
        batch.push_back({entity, "part.retailprice", fields[7], types::AtomType::Canonical});
        batch.push_back({entity, "part.comment", fields[8], types::AtomType::Canonical});

        row_count++;

        if (batch.size() >= 90000) {
            batch_append(store, batch);
            batch.clear();
        }

        if (row_count % 50000 == 0) {
            std::cout << "  Processed " << row_count << " parts...\r" << std::flush;
        }
    }

    if (!batch.empty()) {
        batch_append(store, batch);
    }

    std::cout << "\n  ✓ Imported " << row_count << " parts\n";
    return row_count;
}

// Import PARTSUPP table (800K × SF rows)
// Format: PS_PARTKEY|PS_SUPPKEY|PS_AVAILQTY|PS_SUPPLYCOST|PS_COMMENT|
size_t import_partsupp(core::AtomStore& store, const std::string& filename) {
    std::cout << "Importing PARTSUPP from: " << filename << "\n";

    std::ifstream file(filename);
    file.rdbuf()->pubsetbuf(nullptr, 1024 * 1024); // 1MB buffer for very large files
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open " << filename << "\n";
        return 0;
    }

    size_t row_count = 0;
    std::string line;
    std::vector<std::string> fields;
    fields.reserve(5);
    std::vector<BatchAtom> batch;
    batch.reserve(5000);

    while (std::getline(file, line)) {
        if (line.empty()) continue;

        parse_tbl_line(line, fields);
        if (fields.size() < 5) continue;

        // Create composite key: partkey * 10000 + suppkey
        int64_t partkey = std::stoll(fields[0]);
        int64_t suppkey = std::stoll(fields[1]);
        int64_t composite_key = partkey * 10000 + suppkey;

        types::EntityId entity = create_entity_id("partsupp", composite_key);

        batch.push_back({entity, "partsupp.partkey", fields[0], types::AtomType::Canonical});
        batch.push_back({entity, "partsupp.suppkey", fields[1], types::AtomType::Canonical});
        batch.push_back({entity, "partsupp.availqty", fields[2], types::AtomType::Canonical});
        batch.push_back({entity, "partsupp.supplycost", fields[3], types::AtomType::Canonical});
        batch.push_back({entity, "partsupp.comment", fields[4], types::AtomType::Canonical});

        row_count++;

        if (batch.size() >= 50000) {
            batch_append(store, batch);
            batch.clear();
        }

        if (row_count % 100000 == 0) {
            std::cout << "  Processed " << row_count << " partsupp records...\r" << std::flush;
        }
    }

    if (!batch.empty()) {
        batch_append(store, batch);
    }

    std::cout << "\n  ✓ Imported " << row_count << " partsupp records\n";
    return row_count;
}

// Import ORDERS table (1.5M × SF rows)
// Format: O_ORDERKEY|O_CUSTKEY|O_ORDERSTATUS|O_TOTALPRICE|O_ORDERDATE|O_ORDERPRIORITY|O_CLERK|O_SHIPPRIORITY|O_COMMENT|
size_t import_orders(core::AtomStore& store, const std::string& filename) {
    std::cout << "Importing ORDERS from: " << filename << "\n";

    std::ifstream file(filename);
    file.rdbuf()->pubsetbuf(nullptr, 1024 * 1024); // 1MB buffer for very large files
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open " << filename << "\n";
        return 0;
    }

    size_t row_count = 0;
    std::string line;
    std::vector<std::string> fields;
    fields.reserve(9);
    std::vector<BatchAtom> batch;
    batch.reserve(9000);

    while (std::getline(file, line)) {
        if (line.empty()) continue;

        parse_tbl_line(line, fields);
        if (fields.size() < 9) continue;

        int64_t orderkey = std::stoll(fields[0]);
        types::EntityId entity = create_entity_id("orders", orderkey);

        batch.push_back({entity, "orders.orderkey", fields[0], types::AtomType::Canonical});
        batch.push_back({entity, "orders.custkey", fields[1], types::AtomType::Canonical});
        batch.push_back({entity, "orders.orderstatus", fields[2], types::AtomType::Canonical});
        batch.push_back({entity, "orders.totalprice", fields[3], types::AtomType::Canonical});
        batch.push_back({entity, "orders.orderdate", fields[4], types::AtomType::Canonical});
        batch.push_back({entity, "orders.orderpriority", fields[5], types::AtomType::Canonical});
        batch.push_back({entity, "orders.clerk", fields[6], types::AtomType::Canonical});
        batch.push_back({entity, "orders.shippriority", fields[7], types::AtomType::Canonical});
        batch.push_back({entity, "orders.comment", fields[8], types::AtomType::Canonical});

        row_count++;

        if (batch.size() >= 90000) {
            batch_append(store, batch);
            batch.clear();
        }

        if (row_count % 100000 == 0) {
            std::cout << "  Processed " << row_count << " orders...\r" << std::flush;
        }
    }

    if (!batch.empty()) {
        batch_append(store, batch);
    }

    std::cout << "\n  ✓ Imported " << row_count << " orders\n";
    return row_count;
}

// Import LINEITEM table (6M × SF rows) - LARGEST TABLE
// Format: L_ORDERKEY|L_PARTKEY|L_SUPPKEY|L_LINENUMBER|L_QUANTITY|L_EXTENDEDPRICE|L_DISCOUNT|L_TAX|L_RETURNFLAG|L_LINESTATUS|L_SHIPDATE|L_COMMITDATE|L_RECEIPTDATE|L_SHIPINSTRUCT|L_SHIPMODE|L_COMMENT|
size_t import_lineitem(core::AtomStore& store, const std::string& filename) {
    std::cout << "Importing LINEITEM from: " << filename << " (this is the largest table, may take a while...)\n";

    std::ifstream file(filename);
    file.rdbuf()->pubsetbuf(nullptr, 2048 * 1024); // 2MB buffer for the largest file
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open " << filename << "\n";
        return 0;
    }

    size_t row_count = 0;
    std::string line;
    std::vector<std::string> fields;
    fields.reserve(16);
    std::vector<BatchAtom> batch;
    batch.reserve(16000);

    while (std::getline(file, line)) {
        if (line.empty()) continue;

        parse_tbl_line(line, fields);
        if (fields.size() < 16) continue;

        // Create composite key: orderkey * 10 + linenumber
        int64_t orderkey = std::stoll(fields[0]);
        int64_t linenumber = std::stoll(fields[3]);
        int64_t composite_key = orderkey * 10 + linenumber;

        types::EntityId entity = create_entity_id("lineitem", composite_key);

        batch.push_back({entity, "lineitem.orderkey", fields[0], types::AtomType::Canonical});
        batch.push_back({entity, "lineitem.partkey", fields[1], types::AtomType::Canonical});
        batch.push_back({entity, "lineitem.suppkey", fields[2], types::AtomType::Canonical});
        batch.push_back({entity, "lineitem.linenumber", fields[3], types::AtomType::Canonical});
        batch.push_back({entity, "lineitem.quantity", fields[4], types::AtomType::Canonical});
        batch.push_back({entity, "lineitem.extendedprice", fields[5], types::AtomType::Canonical});
        batch.push_back({entity, "lineitem.discount", fields[6], types::AtomType::Canonical});
        batch.push_back({entity, "lineitem.tax", fields[7], types::AtomType::Canonical});
        batch.push_back({entity, "lineitem.returnflag", fields[8], types::AtomType::Canonical});
        batch.push_back({entity, "lineitem.linestatus", fields[9], types::AtomType::Canonical});
        batch.push_back({entity, "lineitem.shipdate", fields[10], types::AtomType::Canonical});
        batch.push_back({entity, "lineitem.commitdate", fields[11], types::AtomType::Canonical});
        batch.push_back({entity, "lineitem.receiptdate", fields[12], types::AtomType::Canonical});
        batch.push_back({entity, "lineitem.shipinstruct", fields[13], types::AtomType::Canonical});
        batch.push_back({entity, "lineitem.shipmode", fields[14], types::AtomType::Canonical});
        batch.push_back({entity, "lineitem.comment", fields[15], types::AtomType::Canonical});

        row_count++;

        if (batch.size() >= 160000) {
            batch_append(store, batch);
            batch.clear();
        }

        if (row_count % 100000 == 0) {
            std::cout << "  Processed " << row_count << " line items...\r" << std::flush;
        }
    }

    if (!batch.empty()) {
        batch_append(store, batch);
    }

    std::cout << "\n  ✓ Imported " << row_count << " line items\n";
    return row_count;
}

int main(int argc, char* argv[]) {
    std::cout << "=== TPC-H Data Importer for GTAF ===\n\n";

    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <tpch_data_directory> [output_file]\n";
        std::cerr << "\nExample:\n";
        std::cerr << "  " << argv[0] << " ./tpch-data tpch_sf1.dat\n\n";
        std::cerr << "The TPC-H data directory should contain .tbl files:\n";
        std::cerr << "  - region.tbl\n";
        std::cerr << "  - nation.tbl\n";
        std::cerr << "  - supplier.tbl\n";
        std::cerr << "  - customer.tbl\n";
        std::cerr << "  - part.tbl\n";
        std::cerr << "  - partsupp.tbl\n";
        std::cerr << "  - orders.tbl\n";
        std::cerr << "  - lineitem.tbl\n";
        return 1;
    }

    std::string data_dir = argv[1];
    std::string output_file = argc > 2 ? argv[2] : "tpch_import.dat";

    // Add trailing slash if needed
    if (data_dir.back() != '/') {
        data_dir += '/';
    }

    size_t mem_start = get_memory_usage_kb();
    std::cout << "Initial memory: " << format_memory(mem_start) << "\n\n";

    auto start_time = std::chrono::high_resolution_clock::now();

    // Create atom store
    core::AtomStore store;

    // Import tables in order (smallest to largest)
    std::cout << "=== Importing TPC-H Tables ===\n\n";

    size_t total_rows = 0;

    total_rows += import_region(store, data_dir + "region.tbl");
    total_rows += import_nation(store, data_dir + "nation.tbl");
    total_rows += import_supplier(store, data_dir + "supplier.tbl");
    total_rows += import_customer(store, data_dir + "customer.tbl");
    total_rows += import_part(store, data_dir + "part.tbl");
    total_rows += import_partsupp(store, data_dir + "partsupp.tbl");
    total_rows += import_orders(store, data_dir + "orders.tbl");
    total_rows += import_lineitem(store, data_dir + "lineitem.tbl");

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time);

    size_t mem_after = get_memory_usage_kb();
    size_t mem_delta = mem_after - mem_start;

    std::cout << "\n=== Import Summary ===\n";
    std::cout << "Total rows imported: " << total_rows << "\n";
    std::cout << "Total atoms created: " << store.all().size() << "\n";
    std::cout << "Import time: " << duration.count() << " seconds\n";
    std::cout << "Memory used: " << format_memory(mem_delta) << "\n";
    std::cout << "Final memory: " << format_memory(mem_after) << "\n\n";

    // Show deduplication stats
    auto stats = store.get_stats();
    std::cout << "=== Deduplication Statistics ===\n";
    std::cout << "Total atoms: " << stats.total_atoms << "\n";
    std::cout << "Canonical atoms: " << stats.canonical_atoms << "\n";
    std::cout << "Unique canonical: " << stats.unique_canonical_atoms << "\n";
    std::cout << "Deduplication rate: "
              << (stats.canonical_atoms > 0
                  ? (100.0 * stats.deduplicated_hits / stats.canonical_atoms)
                  : 0.0)
              << "%\n\n";

    // Save to file
    std::cout << "Saving to: " << output_file << "\n";
    if (store.save(output_file)) {
        std::cout << "  ✓ Saved successfully\n";
    } else {
        std::cerr << "  ✗ Error saving file\n";
        return 1;
    }

    std::cout << "\n=== Import Complete ===\n";
    std::cout << "You can now query this data using:\n";
    std::cout << "  ./gtaf_tpch_query " << output_file << "\n";

    return 0;
}
