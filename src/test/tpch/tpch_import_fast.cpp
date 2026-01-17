#include "../../core/atom_log.h"
#include "../../types/hash_utils.h"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>
#include <cstring>

using namespace gtaf;

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

std::string format_memory(size_t kb) {
    if (kb >= 1024 * 1024) {
        return std::to_string(kb / (1024 * 1024)) + " GB (" + std::to_string(kb) + " KB)";
    } else if (kb >= 1024) {
        return std::to_string(kb / 1024) + " MB (" + std::to_string(kb) + " KB)";
    } else {
        return std::to_string(kb) + " KB";
    }
}

// Pre-computed entity ID using simple byte packing (no hash)
inline types::EntityId create_entity_id_fast(uint64_t table_id, int64_t key) {
    types::EntityId entity{};
    std::memcpy(entity.bytes.data(), &table_id, 8);
    std::memcpy(entity.bytes.data() + 8, &key, 8);
    return entity;
}

// Table IDs (pre-computed constants)
constexpr uint64_t TABLE_REGION = 1;
constexpr uint64_t TABLE_NATION = 2;
constexpr uint64_t TABLE_SUPPLIER = 3;
constexpr uint64_t TABLE_CUSTOMER = 4;
constexpr uint64_t TABLE_PART = 5;
constexpr uint64_t TABLE_PARTSUPP = 6;
constexpr uint64_t TABLE_ORDERS = 7;
constexpr uint64_t TABLE_LINEITEM = 8;

// Fast field parsing using string_view
class FastLineParser {
public:
    FastLineParser() { fields_.reserve(20); }

    const std::vector<std::string_view>& parse(const std::string& line) {
        fields_.clear();
        size_t start = 0;
        for (size_t pos = 0; pos < line.size(); ++pos) {
            if (line[pos] == '|') {
                fields_.emplace_back(line.data() + start, pos - start);
                start = pos + 1;
            }
        }
        if (start < line.size()) {
            fields_.emplace_back(line.data() + start, line.size() - start);
        }
        return fields_;
    }

private:
    std::vector<std::string_view> fields_;
};

// Batch size for optimal performance
constexpr size_t BATCH_SIZE = 50000;

// Helper to add atom to batch
inline void add_to_batch(std::vector<core::AtomLog::BatchAtom>& batch,
                         types::EntityId entity, const char* tag, std::string_view value) {
    batch.push_back({entity, tag, std::string(value), types::AtomType::Canonical});
}

size_t import_region_fast(core::AtomLog& log, const std::string& filename) {
    std::cout << "Importing REGION from: " << filename << "\n";
    auto start_time = std::chrono::high_resolution_clock::now();

    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open " << filename << "\n";
        return 0;
    }

    FastLineParser parser;
    std::vector<core::AtomLog::BatchAtom> batch;
    batch.reserve(BATCH_SIZE);

    size_t row_count = 0;
    std::string line;
    line.reserve(512);

    while (std::getline(file, line)) {
        if (line.empty()) continue;

        const auto& fields = parser.parse(line);
        if (fields.size() < 3) continue;

        int64_t regionkey = std::stoll(std::string(fields[0]));
        types::EntityId entity = create_entity_id_fast(TABLE_REGION, regionkey);

        add_to_batch(batch, entity, "region.regionkey", fields[0]);
        add_to_batch(batch, entity, "region.name", fields[1]);
        add_to_batch(batch, entity, "region.comment", fields[2]);

        row_count++;
    }

    log.append_batch(batch);
    auto end_time = std::chrono::high_resolution_clock::now();
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    std::cout << "  Imported " << row_count << " regions in " << elapsed_ms << " ms\n";
    return row_count;
}

size_t import_nation_fast(core::AtomLog& log, const std::string& filename) {
    std::cout << "Importing NATION from: " << filename << "\n";
    auto start_time = std::chrono::high_resolution_clock::now();

    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open " << filename << "\n";
        return 0;
    }

    FastLineParser parser;
    std::vector<core::AtomLog::BatchAtom> batch;
    batch.reserve(BATCH_SIZE);

    size_t row_count = 0;
    std::string line;
    line.reserve(512);

    while (std::getline(file, line)) {
        if (line.empty()) continue;

        const auto& fields = parser.parse(line);
        if (fields.size() < 4) continue;

        int64_t nationkey = std::stoll(std::string(fields[0]));
        types::EntityId entity = create_entity_id_fast(TABLE_NATION, nationkey);

        add_to_batch(batch, entity, "nation.nationkey", fields[0]);
        add_to_batch(batch, entity, "nation.name", fields[1]);
        add_to_batch(batch, entity, "nation.regionkey", fields[2]);
        add_to_batch(batch, entity, "nation.comment", fields[3]);

        row_count++;
    }

    log.append_batch(batch);
    auto end_time = std::chrono::high_resolution_clock::now();
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    std::cout << "  Imported " << row_count << " nations in " << elapsed_ms << " ms\n";
    return row_count;
}

size_t import_supplier_fast(core::AtomLog& log, const std::string& filename) {
    std::cout << "Importing SUPPLIER from: " << filename << "\n";
    auto start_time = std::chrono::high_resolution_clock::now();

    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open " << filename << "\n";
        return 0;
    }

    FastLineParser parser;
    std::vector<core::AtomLog::BatchAtom> batch;
    batch.reserve(BATCH_SIZE * 7);

    size_t row_count = 0;
    std::string line;
    line.reserve(1024);

    while (std::getline(file, line)) {
        if (line.empty()) continue;

        const auto& fields = parser.parse(line);
        if (fields.size() < 7) continue;

        int64_t suppkey = std::stoll(std::string(fields[0]));
        types::EntityId entity = create_entity_id_fast(TABLE_SUPPLIER, suppkey);

        add_to_batch(batch, entity, "supplier.suppkey", fields[0]);
        add_to_batch(batch, entity, "supplier.name", fields[1]);
        add_to_batch(batch, entity, "supplier.address", fields[2]);
        add_to_batch(batch, entity, "supplier.nationkey", fields[3]);
        add_to_batch(batch, entity, "supplier.phone", fields[4]);
        add_to_batch(batch, entity, "supplier.acctbal", fields[5]);
        add_to_batch(batch, entity, "supplier.comment", fields[6]);

        row_count++;

        if (batch.size() >= BATCH_SIZE * 7) {
            log.append_batch(batch);
            batch.clear();
            std::cout << "  Processed " << row_count << " suppliers...\r" << std::flush;
        }
    }

    if (!batch.empty()) {
        log.append_batch(batch);
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    std::cout << "\n  Imported " << row_count << " suppliers in " << elapsed_ms << " ms\n";
    return row_count;
}

size_t import_customer_fast(core::AtomLog& log, const std::string& filename) {
    std::cout << "Importing CUSTOMER from: " << filename << "\n";
    auto start_time = std::chrono::high_resolution_clock::now();

    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open " << filename << "\n";
        return 0;
    }

    FastLineParser parser;
    std::vector<core::AtomLog::BatchAtom> batch;
    batch.reserve(BATCH_SIZE * 8);

    size_t row_count = 0;
    std::string line;
    line.reserve(1024);

    while (std::getline(file, line)) {
        if (line.empty()) continue;

        const auto& fields = parser.parse(line);
        if (fields.size() < 8) continue;

        int64_t custkey = std::stoll(std::string(fields[0]));
        types::EntityId entity = create_entity_id_fast(TABLE_CUSTOMER, custkey);

        add_to_batch(batch, entity, "customer.custkey", fields[0]);
        add_to_batch(batch, entity, "customer.name", fields[1]);
        add_to_batch(batch, entity, "customer.address", fields[2]);
        add_to_batch(batch, entity, "customer.nationkey", fields[3]);
        add_to_batch(batch, entity, "customer.phone", fields[4]);
        add_to_batch(batch, entity, "customer.acctbal", fields[5]);
        add_to_batch(batch, entity, "customer.mktsegment", fields[6]);
        add_to_batch(batch, entity, "customer.comment", fields[7]);

        row_count++;

        if (batch.size() >= BATCH_SIZE * 8) {
            log.append_batch(batch);
            batch.clear();
            std::cout << "  Processed " << row_count << " customers...\r" << std::flush;
        }
    }

    if (!batch.empty()) {
        log.append_batch(batch);
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    std::cout << "\n  Imported " << row_count << " customers in " << elapsed_ms << " ms\n";
    return row_count;
}

size_t import_part_fast(core::AtomLog& log, const std::string& filename) {
    std::cout << "Importing PART from: " << filename << "\n";
    auto start_time = std::chrono::high_resolution_clock::now();

    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open " << filename << "\n";
        return 0;
    }

    FastLineParser parser;
    std::vector<core::AtomLog::BatchAtom> batch;
    batch.reserve(BATCH_SIZE * 9);

    size_t row_count = 0;
    std::string line;
    line.reserve(1024);

    while (std::getline(file, line)) {
        if (line.empty()) continue;

        const auto& fields = parser.parse(line);
        if (fields.size() < 9) continue;

        int64_t partkey = std::stoll(std::string(fields[0]));
        types::EntityId entity = create_entity_id_fast(TABLE_PART, partkey);

        add_to_batch(batch, entity, "part.partkey", fields[0]);
        add_to_batch(batch, entity, "part.name", fields[1]);
        add_to_batch(batch, entity, "part.mfgr", fields[2]);
        add_to_batch(batch, entity, "part.brand", fields[3]);
        add_to_batch(batch, entity, "part.type", fields[4]);
        add_to_batch(batch, entity, "part.size", fields[5]);
        add_to_batch(batch, entity, "part.container", fields[6]);
        add_to_batch(batch, entity, "part.retailprice", fields[7]);
        add_to_batch(batch, entity, "part.comment", fields[8]);

        row_count++;

        if (batch.size() >= BATCH_SIZE * 9) {
            log.append_batch(batch);
            batch.clear();
            std::cout << "  Processed " << row_count << " parts...\r" << std::flush;
        }
    }

    if (!batch.empty()) {
        log.append_batch(batch);
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    std::cout << "\n  Imported " << row_count << " parts in " << elapsed_ms << " ms\n";
    return row_count;
}

size_t import_partsupp_fast(core::AtomLog& log, const std::string& filename) {
    std::cout << "Importing PARTSUPP from: " << filename << "\n";
    auto start_time = std::chrono::high_resolution_clock::now();

    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open " << filename << "\n";
        return 0;
    }

    FastLineParser parser;
    std::vector<core::AtomLog::BatchAtom> batch;
    batch.reserve(BATCH_SIZE * 5);

    size_t row_count = 0;
    std::string line;
    line.reserve(1024);

    while (std::getline(file, line)) {
        if (line.empty()) continue;

        const auto& fields = parser.parse(line);
        if (fields.size() < 5) continue;

        int64_t partkey = std::stoll(std::string(fields[0]));
        int64_t suppkey = std::stoll(std::string(fields[1]));
        int64_t composite_key = partkey * 100000 + suppkey;

        types::EntityId entity = create_entity_id_fast(TABLE_PARTSUPP, composite_key);

        add_to_batch(batch, entity, "partsupp.partkey", fields[0]);
        add_to_batch(batch, entity, "partsupp.suppkey", fields[1]);
        add_to_batch(batch, entity, "partsupp.availqty", fields[2]);
        add_to_batch(batch, entity, "partsupp.supplycost", fields[3]);
        add_to_batch(batch, entity, "partsupp.comment", fields[4]);

        row_count++;

        if (batch.size() >= BATCH_SIZE * 5) {
            log.append_batch(batch);
            batch.clear();
            std::cout << "  Processed " << row_count << " partsupp...\r" << std::flush;
        }
    }

    if (!batch.empty()) {
        log.append_batch(batch);
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    std::cout << "\n  Imported " << row_count << " partsupp records in " << elapsed_ms << " ms\n";
    return row_count;
}

size_t import_orders_fast(core::AtomLog& log, const std::string& filename) {
    std::cout << "Importing ORDERS from: " << filename << "\n";
    auto start_time = std::chrono::high_resolution_clock::now();

    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open " << filename << "\n";
        return 0;
    }

    FastLineParser parser;
    std::vector<core::AtomLog::BatchAtom> batch;
    batch.reserve(BATCH_SIZE * 9);

    size_t row_count = 0;
    std::string line;
    line.reserve(1024);

    while (std::getline(file, line)) {
        if (line.empty()) continue;

        const auto& fields = parser.parse(line);
        if (fields.size() < 9) continue;

        int64_t orderkey = std::stoll(std::string(fields[0]));
        types::EntityId entity = create_entity_id_fast(TABLE_ORDERS, orderkey);

        add_to_batch(batch, entity, "orders.orderkey", fields[0]);
        add_to_batch(batch, entity, "orders.custkey", fields[1]);
        add_to_batch(batch, entity, "orders.orderstatus", fields[2]);
        add_to_batch(batch, entity, "orders.totalprice", fields[3]);
        add_to_batch(batch, entity, "orders.orderdate", fields[4]);
        add_to_batch(batch, entity, "orders.orderpriority", fields[5]);
        add_to_batch(batch, entity, "orders.clerk", fields[6]);
        add_to_batch(batch, entity, "orders.shippriority", fields[7]);
        add_to_batch(batch, entity, "orders.comment", fields[8]);

        row_count++;

        if (batch.size() >= BATCH_SIZE * 9) {
            log.append_batch(batch);
            batch.clear();
            std::cout << "  Processed " << row_count << " orders...\r" << std::flush;
        }
    }

    if (!batch.empty()) {
        log.append_batch(batch);
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    std::cout << "\n  Imported " << row_count << " orders in " << elapsed_ms << " ms\n";
    return row_count;
}

size_t import_lineitem_fast(core::AtomLog& log, const std::string& filename) {
    std::cout << "Importing LINEITEM from: " << filename << " (largest table)\n";

    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open " << filename << "\n";
        return 0;
    }

    // Large buffer for file I/O
    constexpr size_t BUFFER_SIZE = 1024 * 1024;
    std::vector<char> buffer(BUFFER_SIZE);
    file.rdbuf()->pubsetbuf(buffer.data(), BUFFER_SIZE);

    FastLineParser parser;
    std::vector<core::AtomLog::BatchAtom> batch;
    batch.reserve(BATCH_SIZE * 16);

    size_t row_count = 0;
    std::string line;
    line.reserve(2048);

    auto start_time = std::chrono::high_resolution_clock::now();

    while (std::getline(file, line)) {
        if (line.empty()) continue;

        const auto& fields = parser.parse(line);
        if (fields.size() < 16) continue;

        int64_t orderkey = std::stoll(std::string(fields[0]));
        int64_t linenumber = std::stoll(std::string(fields[3]));
        int64_t composite_key = orderkey * 10 + linenumber;

        types::EntityId entity = create_entity_id_fast(TABLE_LINEITEM, composite_key);

        add_to_batch(batch, entity, "lineitem.orderkey", fields[0]);
        add_to_batch(batch, entity, "lineitem.partkey", fields[1]);
        add_to_batch(batch, entity, "lineitem.suppkey", fields[2]);
        add_to_batch(batch, entity, "lineitem.linenumber", fields[3]);
        add_to_batch(batch, entity, "lineitem.quantity", fields[4]);
        add_to_batch(batch, entity, "lineitem.extendedprice", fields[5]);
        add_to_batch(batch, entity, "lineitem.discount", fields[6]);
        add_to_batch(batch, entity, "lineitem.tax", fields[7]);
        add_to_batch(batch, entity, "lineitem.returnflag", fields[8]);
        add_to_batch(batch, entity, "lineitem.linestatus", fields[9]);
        add_to_batch(batch, entity, "lineitem.shipdate", fields[10]);
        add_to_batch(batch, entity, "lineitem.commitdate", fields[11]);
        add_to_batch(batch, entity, "lineitem.receiptdate", fields[12]);
        add_to_batch(batch, entity, "lineitem.shipinstruct", fields[13]);
        add_to_batch(batch, entity, "lineitem.shipmode", fields[14]);
        add_to_batch(batch, entity, "lineitem.comment", fields[15]);

        row_count++;

        if (batch.size() >= BATCH_SIZE * 16) {
            log.append_batch(batch);
            batch.clear();

            auto now = std::chrono::high_resolution_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start_time).count();
            double rate = elapsed > 0 ? row_count / static_cast<double>(elapsed) : 0;
            std::cout << "  Processed " << row_count << " line items ("
                      << static_cast<int>(rate) << " rows/sec)...\r" << std::flush;
        }
    }

    if (!batch.empty()) {
        log.append_batch(batch);
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    std::cout << "\n  Imported " << row_count << " line items in " << elapsed_ms << " ms\n";
    return row_count;
}

int main(int argc, char* argv[]) {
    std::cout << "=== TPC-H Fast Data Importer for GTAF ===\n\n";

    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <tpch_data_directory> [output_file]\n";
        std::cerr << "\nExample:\n";
        std::cerr << "  " << argv[0] << " ./data tpch_sf1.dat\n\n";
        return 1;
    }

    std::string data_dir = argv[1];
    std::string output_file = argc > 2 ? argv[2] : "tpch_import.dat";

    if (data_dir.back() != '/') {
        data_dir += '/';
    }

    size_t mem_start = get_memory_usage_kb();
    std::cout << "Initial memory: " << format_memory(mem_start) << "\n\n";

    auto start_time = std::chrono::high_resolution_clock::now();

    core::AtomLog log;

    // Pre-reserve capacity to avoid rehashing during bulk import
    // TPC-H SF1: ~8.6M atoms, ~1.5M entities (estimate based on table sizes)
    // Atoms: 5 + 25*4 + 10K*7 + 150K*8 + 200K*9 + 800K*5 + 1.5M*9 + 6M*16 ≈ 115M atoms for SF1
    // For smaller dataset, estimate based on expected rows
    constexpr size_t ESTIMATED_ATOMS = 10000000;  // 10M atoms
    constexpr size_t ESTIMATED_ENTITIES = 2000000; // 2M entities
    std::cout << "Pre-allocating memory for ~" << ESTIMATED_ATOMS << " atoms, ~" << ESTIMATED_ENTITIES << " entities...\n";
    log.reserve(ESTIMATED_ATOMS, ESTIMATED_ENTITIES);

    std::cout << "\n=== Importing TPC-H Tables ===\n\n";

    size_t total_rows = 0;

    total_rows += import_region_fast(log, data_dir + "region.tbl");
    total_rows += import_nation_fast(log, data_dir + "nation.tbl");
    total_rows += import_supplier_fast(log, data_dir + "supplier.tbl");
    total_rows += import_customer_fast(log, data_dir + "customer.tbl");
    total_rows += import_part_fast(log, data_dir + "part.tbl");
    total_rows += import_partsupp_fast(log, data_dir + "partsupp.tbl");
    total_rows += import_orders_fast(log, data_dir + "orders.tbl");
    total_rows += import_lineitem_fast(log, data_dir + "lineitem.tbl");

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time);

    size_t mem_after = get_memory_usage_kb();
    size_t mem_delta = mem_after - mem_start;

    std::cout << "\n=== Import Summary ===\n";
    std::cout << "Total rows imported: " << total_rows << "\n";
    std::cout << "Total atoms created: " << log.all().size() << "\n";
    std::cout << "Import time: " << duration.count() << " seconds\n";
    std::cout << "Throughput: " << (duration.count() > 0 ? total_rows / duration.count() : 0) << " rows/sec\n";
    std::cout << "Memory used: " << format_memory(mem_delta) << "\n";
    std::cout << "Final memory: " << format_memory(mem_after) << "\n\n";

    auto stats = log.get_stats();
    std::cout << "=== Deduplication Statistics ===\n";
    std::cout << "Total atoms: " << stats.total_atoms << "\n";
    std::cout << "Canonical atoms: " << stats.canonical_atoms << "\n";
    std::cout << "Unique canonical: " << stats.unique_canonical_atoms << "\n";
    std::cout << "Deduplication rate: "
              << (stats.canonical_atoms > 0
                  ? (100.0 * stats.deduplicated_hits / stats.canonical_atoms)
                  : 0.0)
              << "%\n\n";

    std::cout << "Saving to: " << output_file << "\n";
    if (log.save(output_file)) {
        std::cout << "  Saved successfully\n";
    } else {
        std::cerr << "  Error saving file\n";
        return 1;
    }

    std::cout << "\n=== Import Complete ===\n";

    return 0;
}
