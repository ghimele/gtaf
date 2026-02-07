// gtaf_cli.cpp - Command line interface for GTAF
#include "../core/atom_store.h"
#include "../core/projection_engine.h"
#include "../core/query_index.h"
#include "../core/persistence.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <vector>
#include <string>

using namespace gtaf;
using namespace gtaf::core;
using namespace gtaf::types;

// ---- Helper Functions ----

void print_help() {
    std::cout << "GTAF CLI - Command Line Interface for GTAF Framework\n"
              << "Usage: gtaf_cli <command> [options]\n\n"
              << "Commands:\n"
              << "  load <datafile.dat>                 Load data file into store\n"
              << "  save <datafile.dat>                 Save current store to data file\n"
              << "  index <tag1,tag2,...>               Build indexes for specified tags\n"
              << "  project                             Create projection engine and show stats\n"
              << "  stats                               Show current store statistics\n"
              << "  help                                Show this help message\n\n"
              << "Chained Operations:\n"
              << "  gtaf_cli load data.dat stats        Load then show stats\n"
              << "  gtaf_cli load data.dat project      Load then project\n"
              << "  gtaf_cli load data.dat save out.dat  Load then save\n\n"
              << "Examples:\n"
              << "  gtaf_cli load data.dat              # Load data from file\n"
              << "  gtaf_cli save data.dat              # Save current store\n"
              << "  gtaf_cli index user.name,user.status  # Build indexes\n"
              << "  gtaf_cli project                    # Create projection engine\n"
              << "  gtaf_cli stats                      # Show statistics\n";
}

void print_stats(const AtomStore::Stats& stats) {
    std::cout << "\n=== Store Statistics ===\n"
              << "Total atoms:           " << std::setw(12) << stats.total_atoms << "\n"
              << "Canonical atoms:       " << std::setw(12) << stats.canonical_atoms << "\n"
              << "Unique canonical atoms:" << std::setw(12) << stats.unique_canonical_atoms << "\n"
              << "Total references:      " << std::setw(12) << stats.total_references << "\n"
              << "Total entities:        " << std::setw(12) << stats.total_entities << "\n";
    
    if (stats.canonical_atoms > 0) {
        double dedup_ratio = (double)stats.unique_canonical_atoms / stats.canonical_atoms;
        std::cout << "Deduplication ratio:  " << std::setw(12) << std::fixed << std::setprecision(3) 
                  << dedup_ratio << " (lower is better)\n";
    }
    std::cout << std::endl;
}

void print_index_stats(const QueryIndex::IndexStats& stats) {
    std::cout << "\n=== Index Statistics ===\n"
              << "Indexed tags:          " << std::setw(12) << stats.num_indexed_tags << "\n"
              << "Indexed entities:      " << std::setw(12) << stats.num_indexed_entities << "\n"
              << "Total index entries:   " << std::setw(12) << stats.total_entries << "\n"
              << std::endl;
}

// ---- Command Handlers ----

bool handle_load(AtomStore& store, const std::string& filename) {
    std::cout << "Loading data from: " << filename << "\n";
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    if (!store.load(filename)) {
        std::cerr << "Error: Failed to load data file: " << filename << "\n";
        return false;
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    auto stats = store.get_stats();
    std::cout << "✓ Successfully loaded " << stats.total_atoms << " atoms in " 
              << duration.count() << "ms\n";
    
    print_stats(stats);
    return true;
}

bool handle_save(const AtomStore& store, const std::string& filename) {
    std::cout << "Saving data to: " << filename << "\n";
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    if (!store.save(filename)) {
        std::cerr << "Error: Failed to save data file: " << filename << "\n";
        return false;
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    auto stats = store.get_stats();
    std::cout << "✓ Successfully saved " << stats.total_atoms << " atoms in " 
              << duration.count() << "ms\n";
    return true;
}

bool handle_index(AtomStore& store, const std::string& tags_str) {
    // Parse comma-separated tags
    std::vector<std::string> tags;
    std::stringstream ss(tags_str);
    std::string tag;
    
    while (std::getline(ss, tag, ',')) {
        // Trim whitespace
        tag.erase(0, tag.find_first_not_of(" \t"));
        tag.erase(tag.find_last_not_of(" \t") + 1);
        if (!tag.empty()) {
            tags.push_back(tag);
        }
    }
    
    if (tags.empty()) {
        std::cerr << "Error: No valid tags specified\n";
        return false;
    }
    
    std::cout << "Building indexes for " << tags.size() << " tags:\n";
    for (const auto& tag : tags) {
        std::cout << "  - " << tag << "\n";
    }
    
    // Create query index
    QueryIndex index(store);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    size_t entries_created = index.build_indexes(tags);
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "✓ Built " << entries_created << " index entries in " 
              << duration.count() << "ms\n";
    
    auto index_stats = index.get_stats();
    print_index_stats(index_stats);
    return true;
}

bool handle_project(const AtomStore& store) {
    std::cout << "Creating projection engine...\n";
    
    auto start_time = std::chrono::high_resolution_clock::now();
    ProjectionEngine projector(store);
    
    // Get all entities
    auto entities = projector.get_all_entities();
    auto end_time = std::chrono::high_resolution_clock::now();
    auto setup_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "✓ Projection engine created for " << entities.size() 
              << " entities in " << setup_time.count() << "ms\n";
    
    // Rebuild first few entities as example
    size_t sample_size = std::min(entities.size(), size_t(5));
    std::cout << "\nRebuilding " << sample_size << " sample entities:\n";
    
    start_time = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < sample_size; ++i) {
        Node node = projector.rebuild(entities[i]);
        auto props = node.get_all();
        std::cout << "  Entity " << (i + 1) << ": " << props.size() << " properties\n";
        
        // Show first few properties
        size_t prop_sample = std::min(props.size(), size_t(3));
        auto it = props.begin();
        for (size_t j = 0; j < prop_sample; ++j, ++it) {
            const auto& [tag, value] = *it;
            std::cout << "    " << tag << ": ";
            
            std::visit([](auto&& arg) {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, std::monostate>) {
                    std::cout << "<empty>";
                } else if constexpr (std::is_same_v<T, bool>) {
                    std::cout << (arg ? "true" : "false");
                } else if constexpr (std::is_same_v<T, int64_t>) {
                    std::cout << arg;
                } else if constexpr (std::is_same_v<T, double>) {
                    std::cout << std::fixed << std::setprecision(2) << arg;
                } else if constexpr (std::is_same_v<T, std::string>) {
                    std::cout << "'" << arg << "'";
                } else if constexpr (std::is_same_v<T, std::vector<float>>) {
                    std::cout << "vector[" << arg.size() << "]";
                } else if constexpr (std::is_same_v<T, std::vector<uint8_t>>) {
                    std::cout << "bytes[" << arg.size() << "]";
                } else if constexpr (std::is_same_v<T, EdgeValue>) {
                    std::cout << "edge(" << arg.relation << "->" 
                              << std::hex << std::setw(4) 
                              << *(uint32_t*)arg.target.bytes.data() << "...)";
                }
            }, value);
            
            std::cout << "\n";
        }
        
        if (props.size() > prop_sample) {
            std::cout << "    ... and " << (props.size() - prop_sample) << " more\n";
        }
    }
    end_time = std::chrono::high_resolution_clock::now();
    auto rebuild_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "✓ Sample projection rebuilt in " << rebuild_time.count() << "ms\n";
    std::cout << "Note: Full projection of all " << entities.size() 
              << " entities would take approximately " 
              << (entities.size() > 0 ? (rebuild_time.count() * entities.size() / sample_size) : 0) 
              << "ms\n";
    
    return true;
}

bool handle_stats(const AtomStore& store) {
    auto stats = store.get_stats();
    print_stats(stats);
    return true;
}

// ---- Main ----

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Error: No command specified\n\n";
        print_help();
        return 1;
    }
    
    // Create atom store - this persists across multiple operations
    AtomStore store;
    bool success = true;
    
    // Parse commands sequentially - this allows chaining
    int i = 1;
    while (i < argc) {
        std::string command = argv[i];
        
        // Handle help command
        if (command == "help") {
            print_help();
            return 0;
        }
        
        // Handle load command
        if (command == "load") {
            if (i + 1 >= argc) {
                std::cerr << "Error: load command requires filename\n";
                std::cerr << "Usage: gtaf_cli load <datafile.dat>\n";
                return 1;
            }
            success = handle_load(store, argv[i + 1]);
            i += 2;
            continue;
        }
        
        // Handle save command
        if (command == "save") {
            if (i + 1 >= argc) {
                std::cerr << "Error: save command requires filename\n";
                std::cerr << "Usage: gtaf_cli save <datafile.dat>\n";
                return 1;
            }
            success = handle_save(store, argv[i + 1]);
            i += 2;
            continue;
        }
        
        // Handle index command
        if (command == "index") {
            if (i + 1 >= argc) {
                std::cerr << "Error: index command requires comma-separated tags\n";
                std::cerr << "Usage: gtaf_cli index <tag1,tag2,...>\n";
                return 1;
            }
            success = handle_index(store, argv[i + 1]);
            i += 2;
            continue;
        }
        
        // Handle project command
        if (command == "project") {
            success = handle_project(store);
            i += 1;
            continue;
        }
        
        // Handle stats command
        if (command == "stats") {
            success = handle_stats(store);
            i += 1;
            continue;
        }
        
        // Unknown command
        std::cerr << "Error: Unknown command '" << command << "'\n\n";
        print_help();
        return 1;
    }
    
    return success ? 0 : 1;
}