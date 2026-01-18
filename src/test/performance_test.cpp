#include "core/atom_store.h"
#include "types/hash_utils.h"
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
void batch_append(core::AtomStore& log, const std::vector<BatchAtom>& batch) {
    for (const auto& atom : batch) {
        log.append(atom.entity, atom.tag, atom.value, atom.classification);
    }
}

// Pre-computed table hashes for performance
static const std::unordered_map<std::string, uint64_t> table_hashes = {
    {"region", std::hash<std::string>{}("region")},
    {"nation", std::hash<std::string>{}("nation")}
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

// Test performance on just small tables
int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <tpch_data_directory>\n";
        return 1;
    }

    std::string data_dir = argv[1];
    if (data_dir.back() != '/') {
        data_dir += '/';
    }

    auto start_time = std::chrono::high_resolution_clock::now();

    // Test REGION and NATION only
    core::AtomStore log;
    std::vector<std::string> fields;
    std::vector<BatchAtom> batch;
    batch.reserve(1000);

    // Import REGION
    {
        std::ifstream file(data_dir + "region.tbl");
        file.rdbuf()->pubsetbuf(nullptr, 64 * 1024);
        
        std::string line;
        while (std::getline(file, line)) {
            if (line.empty()) continue;
            
            fields.reserve(3);
            parse_tbl_line(line, fields);
            if (fields.size() < 3) continue;

            int64_t regionkey = std::stoll(fields[0]);
            types::EntityId entity = create_entity_id("region", regionkey);

            batch.push_back({entity, "region.regionkey", fields[0], types::AtomType::Canonical});
            batch.push_back({entity, "region.name", fields[1], types::AtomType::Canonical});
            batch.push_back({entity, "region.comment", fields[2], types::AtomType::Canonical});

            if (batch.size() >= 3000) {
                batch_append(log, batch);
                batch.clear();
            }
        }
        
        if (!batch.empty()) {
            batch_append(log, batch);
            batch.clear();
        }
    }

    // Import NATION
    {
        std::ifstream file(data_dir + "nation.tbl");
        file.rdbuf()->pubsetbuf(nullptr, 64 * 1024);
        
        std::string line;
        while (std::getline(file, line)) {
            if (line.empty()) continue;
            
            fields.reserve(4);
            parse_tbl_line(line, fields);
            if (fields.size() < 4) continue;

            int64_t nationkey = std::stoll(fields[0]);
            types::EntityId entity = create_entity_id("nation", nationkey);

            batch.push_back({entity, "nation.nationkey", fields[0], types::AtomType::Canonical});
            batch.push_back({entity, "nation.name", fields[1], types::AtomType::Canonical});
            batch.push_back({entity, "nation.regionkey", fields[2], types::AtomType::Canonical});
            batch.push_back({entity, "nation.comment", fields[3], types::AtomType::Canonical});

            if (batch.size() >= 4000) {
                batch_append(log, batch);
                batch.clear();
            }
        }
        
        if (!batch.empty()) {
            batch_append(log, batch);
        }
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    std::cout << "Small tables import time: " << duration.count() << " ms\n";
    std::cout << "Total atoms: " << log.all().size() << "\n";

    return 0;
}