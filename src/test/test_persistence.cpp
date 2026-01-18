#include "test_framework.h"
#include "../core/atom_store.h"
#include <algorithm>
#include <cstdio>

using namespace gtaf;
using namespace gtaf::test;

// Helper to create test EntityIds
types::EntityId make_entity_persist(uint8_t id) {
    types::EntityId entity{};
    std::fill(entity.bytes.begin(), entity.bytes.end(), 0);
    entity.bytes[0] = id;
    return entity;
}

TEST(Persistence, SaveAndLoad) {
    std::string filepath = "test_persist.dat";
    auto entity = make_entity_persist(1);

    // Create and populate log
    core::AtomStore log;
    log.append(entity, "name", std::string("Alice"), types::AtomType::Canonical);
    log.append(entity, "age", static_cast<int64_t>(30), types::AtomType::Canonical);
    log.append(entity, "score", 95.5, types::AtomType::Canonical);

    size_t original_count = log.all().size();

    // Save
    ASSERT_TRUE(log.save(filepath));

    // Load into new log
    core::AtomStore loaded_log;
    ASSERT_TRUE(loaded_log.load(filepath));

    // Verify atom count
    ASSERT_EQ(loaded_log.all().size(), original_count);

    // Clean up
    std::remove(filepath.c_str());
}

TEST(Persistence, PreserveStats) {
    std::string filepath = "test_persist_stats.dat";
    auto entity1 = make_entity_persist(1);
    auto entity2 = make_entity_persist(2);

    // Create log with deduplication
    core::AtomStore log;
    log.append(entity1, "status", std::string("active"), types::AtomType::Canonical);
    log.append(entity2, "status", std::string("active"), types::AtomType::Canonical);
    log.append(entity1, "status", std::string("inactive"), types::AtomType::Canonical);

    auto original_stats = log.get_stats();

    // Save and load
    ASSERT_TRUE(log.save(filepath));
    core::AtomStore loaded_log;
    ASSERT_TRUE(loaded_log.load(filepath));

    auto loaded_stats = loaded_log.get_stats();

    // Stats should match
    ASSERT_EQ(loaded_stats.total_atoms, original_stats.total_atoms);
    ASSERT_EQ(loaded_stats.unique_canonical_atoms, original_stats.unique_canonical_atoms);

    std::remove(filepath.c_str());
}

TEST(Persistence, PreserveAtomOrder) {
    std::string filepath = "test_persist_order.dat";
    auto entity = make_entity_persist(1);

    core::AtomStore log;
    log.append(entity, "value", static_cast<int64_t>(1), types::AtomType::Canonical);
    log.append(entity, "value", static_cast<int64_t>(2), types::AtomType::Canonical);
    log.append(entity, "value", static_cast<int64_t>(3), types::AtomType::Canonical);

    ASSERT_TRUE(log.save(filepath));

    core::AtomStore loaded_log;
    ASSERT_TRUE(loaded_log.load(filepath));

    // Check LSN order is preserved in entity references
    const auto* refs = loaded_log.get_entity_atoms(entity);
    ASSERT_TRUE(refs != nullptr);
    ASSERT_EQ(refs->size(), 3);
    ASSERT_TRUE((*refs)[0].lsn.value < (*refs)[1].lsn.value);
    ASSERT_TRUE((*refs)[1].lsn.value < (*refs)[2].lsn.value);

    // Check content atoms are preserved
    const auto& atoms = loaded_log.all();
    ASSERT_EQ(atoms.size(), 3);

    std::remove(filepath.c_str());
}

TEST(Persistence, LargeDataset) {
    std::string filepath = "test_persist_large.dat";
    auto entity = make_entity_persist(1);

    core::AtomStore log;

    // Add 1000 temporal values
    for (int i = 0; i < 1000; ++i) {
        log.append(entity, "sensor.temp", static_cast<double>(20.0 + i), types::AtomType::Temporal);
    }

    ASSERT_TRUE(log.save(filepath));

    core::AtomStore loaded_log;
    ASSERT_TRUE(loaded_log.load(filepath));

    ASSERT_EQ(loaded_log.all().size(), 1000);

    std::remove(filepath.c_str());
}

TEST(Persistence, InvalidFile) {
    core::AtomStore log;

    // Try to load non-existent file
    ASSERT_FALSE(log.load("nonexistent_file.dat"));
}

TEST(Persistence, PreserveEdgeValues) {
    std::string filepath = "test_persist_edges.dat";
    auto entity1 = make_entity_persist(1);
    auto entity2 = make_entity_persist(2);

    core::AtomStore log;
    types::EdgeValue edge{entity2, "follows"};
    log.append(entity1, "edge.follows", edge, types::AtomType::Canonical);

    ASSERT_TRUE(log.save(filepath));

    core::AtomStore loaded_log;
    ASSERT_TRUE(loaded_log.load(filepath));

    const auto& atoms = loaded_log.all();
    ASSERT_EQ(atoms.size(), 1);
    ASSERT_TRUE(std::holds_alternative<types::EdgeValue>(atoms[0].value()));

    auto& loaded_edge = std::get<types::EdgeValue>(atoms[0].value());
    ASSERT_EQ(loaded_edge.target, entity2);
    ASSERT_EQ(loaded_edge.relation, "follows");

    std::remove(filepath.c_str());
}

TEST(Persistence, PreserveAllValueTypes) {
    std::string filepath = "test_persist_types.dat";
    auto entity = make_entity_persist(1);

    core::AtomStore log;
    log.append(entity, "bool_val", true, types::AtomType::Canonical);
    log.append(entity, "int_val", static_cast<int64_t>(42), types::AtomType::Canonical);
    log.append(entity, "double_val", 3.14, types::AtomType::Canonical);
    log.append(entity, "string_val", std::string("hello"), types::AtomType::Canonical);

    std::vector<float> vec = {1.0f, 2.0f, 3.0f};
    log.append(entity, "vec_val", vec, types::AtomType::Canonical);

    ASSERT_TRUE(log.save(filepath));

    core::AtomStore loaded_log;
    ASSERT_TRUE(loaded_log.load(filepath));

    const auto& atoms = loaded_log.all();
    ASSERT_EQ(atoms.size(), 5);

    // Verify types are preserved
    ASSERT_TRUE(std::holds_alternative<bool>(atoms[0].value()));
    ASSERT_TRUE(std::holds_alternative<int64_t>(atoms[1].value()));
    ASSERT_TRUE(std::holds_alternative<double>(atoms[2].value()));
    ASSERT_TRUE(std::holds_alternative<std::string>(atoms[3].value()));
    ASSERT_TRUE(std::holds_alternative<std::vector<float>>(atoms[4].value()));

    // Verify values
    ASSERT_EQ(std::get<bool>(atoms[0].value()), true);
    ASSERT_EQ(std::get<int64_t>(atoms[1].value()), 42);
    ASSERT_EQ(std::get<double>(atoms[2].value()), 3.14);
    ASSERT_EQ(std::get<std::string>(atoms[3].value()), "hello");
    ASSERT_EQ(std::get<std::vector<float>>(atoms[4].value()).size(), 3);

    std::remove(filepath.c_str());
}
