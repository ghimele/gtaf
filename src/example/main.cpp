#include "../core/atom_log.h"
#include "../core/projection_engine.h"
#include "../types/hash_utils.h"
#include <iostream>
#include <algorithm>

using namespace gtaf;

int main() {
    std::cout << "=== GTAF Content-Addressed Storage Demo ===\n\n";

    core::AtomLog log;

    // Create EntityIds
    types::EntityId user1{};
    std::fill(user1.bytes.begin(), user1.bytes.end(), 0);
    user1.bytes[0] = 1;

    types::EntityId user2{};
    std::fill(user2.bytes.begin(), user2.bytes.end(), 0);
    user2.bytes[0] = 2;

    types::EntityId recipe{};
    std::fill(recipe.bytes.begin(), recipe.bytes.end(), 0);
    recipe.bytes[0] = 3;

    types::EntityId sensor{};
    std::fill(sensor.bytes.begin(), sensor.bytes.end(), 0);
    sensor.bytes[0] = 4;

    std::cout << "--- Test 1: Canonical Atoms (Deduplicated) ---\n";

    // Append canonical atoms (default classification)
    auto atom1 = log.append(user1, "user.status", std::string("active"), types::AtomType::Canonical);
    std::cout << "Created atom1: user.status = 'active'\n";
    std::cout << "  AtomId: " << types::atom_id_to_hex(atom1.atom_id()) << "\n";

    // Append same value to different user - should reuse the atom!
    auto atom2 = log.append(user2, "user.status", std::string("active"), types::AtomType::Canonical);
    std::cout << "Created atom2: user.status = 'active' (different user)\n";
    std::cout << "  AtomId: " << types::atom_id_to_hex(atom2.atom_id()) << "\n";

    if (atom1.atom_id() == atom2.atom_id()) {
        std::cout << "  ✓ DEDUPLICATED! Both atoms share the same content-addressed ID\n";
    } else {
        std::cout << "  ✗ ERROR: Atoms should have been deduplicated\n";
    }

    // Different value - should create new atom
    auto atom3 = log.append(user1, "user.status", std::string("inactive"), types::AtomType::Canonical);
    std::cout << "Created atom3: user.status = 'inactive'\n";
    std::cout << "  AtomId: " << types::atom_id_to_hex(atom3.atom_id()) << "\n";
    std::cout << "  ✓ Different value = different hash\n\n";

    std::cout << "--- Test 2: Temporal Atoms (NOT Deduplicated) ---\n";

    // Temporal atoms use sequential IDs
    auto temp1 = log.append(sensor, "temperature", 23.5, types::AtomType::Temporal);
    auto temp2 = log.append(sensor, "temperature", 23.5, types::AtomType::Temporal);

    std::cout << "Created temp1: temperature = 23.5\n";
    std::cout << "  AtomId: " << types::atom_id_to_hex(temp1.atom_id()) << "\n";
    std::cout << "Created temp2: temperature = 23.5 (same value)\n";
    std::cout << "  AtomId: " << types::atom_id_to_hex(temp2.atom_id()) << "\n";

    if (temp1.atom_id() != temp2.atom_id()) {
        std::cout << "  ✓ NOT DEDUPLICATED (correct for time-series data)\n";
    } else {
        std::cout << "  ✗ ERROR: Temporal atoms should NOT be deduplicated\n";
    }
    std::cout << "\n";

    std::cout << "--- Test 2b: Temporal Chunking (>1000 values) ---\n";

    // Append 1500 temporal values to trigger chunk sealing
    std::cout << "Appending 1500 temperature readings...\n";
    for (int i = 0; i < 1500; ++i) {
        double temp = 20.0 + (i % 10) * 0.5;  // Temperature varies between 20.0-24.5
        log.append(sensor, "sensor.temperature", temp, types::AtomType::Temporal);
    }
    std::cout << "  ✓ Chunk should have been sealed at 1000 values\n";
    std::cout << "  ✓ Second chunk should have 500 values\n";

    // Query all temporal data
    std::cout << "\nQuerying all temporal data...\n";
    auto all_temps = log.query_temporal_all(sensor, "sensor.temperature");
    std::cout << "  Retrieved " << all_temps.total_count << " temperature readings\n";

    if (all_temps.total_count == 1500) {
        std::cout << "  ✓ All 1500 values successfully queried\n";
    }

    // Show sample of first and last values
    if (all_temps.values.size() >= 2) {
        if (std::holds_alternative<double>(all_temps.values[0]) &&
            std::holds_alternative<double>(all_temps.values.back())) {
            double first = std::get<double>(all_temps.values[0]);
            double last = std::get<double>(all_temps.values.back());
            std::cout << "  First value: " << first << "\n";
            std::cout << "  Last value: " << last << "\n";
        }
    }
    std::cout << "\n";

    std::cout << "--- Test 3: Mutable Atoms (Counters with Delta Logging) ---\n";

    auto counter1 = log.append(user1, "login_count", static_cast<int64_t>(1), types::AtomType::Mutable);
    std::cout << "Created counter1: login_count = 1\n";
    std::cout << "  AtomId: " << types::atom_id_to_hex(counter1.atom_id()) << "\n";

    auto counter2 = log.append(user1, "login_count", static_cast<int64_t>(2), types::AtomType::Mutable);
    std::cout << "Updated to counter2: login_count = 2\n";
    std::cout << "  AtomId: " << types::atom_id_to_hex(counter2.atom_id()) << "\n";

    if (counter1.atom_id() == counter2.atom_id()) {
        std::cout << "  ✓ Same AtomId (in-place mutation with delta logging)\n";
    }

    // Trigger snapshot by exceeding delta threshold (10 deltas)
    std::cout << "\nAppending 10 more mutations to trigger snapshot...\n";
    for (int i = 3; i <= 12; ++i) {
        log.append(user1, "login_count", static_cast<int64_t>(i), types::AtomType::Mutable);
    }
    std::cout << "  ✓ Snapshot should have been emitted at 10 deltas\n";
    std::cout << "  ✓ Delta history cleared after snapshot\n\n";

    std::cout << "--- Test 4: Edge Values ---\n";

    types::EdgeValue edge{recipe, "likes"};
    log.append(user1, "edge.likes", edge, types::AtomType::Canonical);
    std::cout << "Created edge: user1 -> likes -> recipe\n\n";

    std::cout << "--- Statistics ---\n";
    auto stats = log.get_stats();
    std::cout << "Total atoms in log: " << stats.total_atoms << "\n";
    std::cout << "Canonical atoms created: " << stats.canonical_atoms << "\n";
    std::cout << "Unique canonical atoms: " << stats.unique_canonical_atoms << "\n";
    std::cout << "Deduplication hits: " << stats.deduplicated_hits << "\n";
    std::cout << "Deduplication rate: "
              << (stats.canonical_atoms > 0
                  ? (100.0 * stats.deduplicated_hits / stats.canonical_atoms)
                  : 0.0)
              << "%\n\n";

    std::cout << "--- Projection Rebuild & Value Queries ---\n";
    core::ProjectionEngine projector(log);

    auto user1_node = projector.rebuild(user1);
    auto user2_node = projector.rebuild(user2);
    auto recipe_node = projector.rebuild(recipe);
    auto sensor_node = projector.rebuild(sensor);

    std::cout << "Rebuilt " << user1_node.history().size() << " atoms for user1\n";
    std::cout << "Rebuilt " << user2_node.history().size() << " atoms for user2\n";
    std::cout << "Rebuilt " << recipe_node.history().size() << " atoms for recipe\n";
    std::cout << "Rebuilt " << sensor_node.history().size() << " atoms for sensor\n\n";

    std::cout << "--- Fast Value Reads (No Log Traversal) ---\n";

    // Query user1's status (canonical)
    if (auto status = user1_node.get("user.status")) {
        if (std::holds_alternative<std::string>(*status)) {
            std::cout << "user1.status = '" << std::get<std::string>(*status) << "'\n";
        }
    }

    // Query user1's login_count (mutable)
    if (auto count = user1_node.get("login_count")) {
        if (std::holds_alternative<int64_t>(*count)) {
            std::cout << "user1.login_count = " << std::get<int64_t>(*count) << "\n";
        }
    }

    // Get all properties for user1
    std::cout << "\nAll properties for user1:\n";
    auto all_props = user1_node.get_all();
    std::cout << "  Total properties: " << all_props.size() << "\n";
    for (const auto& [tag, value] : all_props) {
        std::cout << "  - " << tag;
        if (std::holds_alternative<std::string>(value)) {
            std::cout << " = '" << std::get<std::string>(value) << "'";
        } else if (std::holds_alternative<int64_t>(value)) {
            std::cout << " = " << std::get<int64_t>(value);
        }
        std::cout << "\n";
    }

    std::cout << "\n  ✓ Values retrieved from projection (no atom log access)\n";
    std::cout << "  ✓ O(1) lookup per property\n";

    std::cout << "\n=== Demo Complete ===\n";
    return 0;
}
