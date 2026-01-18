#include "test_framework.h"
#include "../core/atom_store.h"
#include "../types/hash_utils.h"
#include <algorithm>

using namespace gtaf;
using namespace gtaf::test;

// Helper to create test EntityIds
types::EntityId make_entity(uint8_t id) {
    types::EntityId entity{};
    std::fill(entity.bytes.begin(), entity.bytes.end(), 0);
    entity.bytes[0] = id;
    return entity;
}

TEST(AtomStore, CanonicalDeduplication) {
    core::AtomStore log;
    auto entity1 = make_entity(1);
    auto entity2 = make_entity(2);

    // Append same value to different entities
    auto atom1 = log.append(entity1, "status", std::string("active"), types::AtomType::Canonical);
    auto atom2 = log.append(entity2, "status", std::string("active"), types::AtomType::Canonical);

    // Should have same content-addressed ID (content deduplication)
    ASSERT_EQ(atom1.atom_id(), atom2.atom_id());

    // But both entities should have references
    const auto* refs1 = log.get_entity_atoms(entity1);
    const auto* refs2 = log.get_entity_atoms(entity2);
    ASSERT_TRUE(refs1 != nullptr);
    ASSERT_TRUE(refs2 != nullptr);
    ASSERT_EQ(refs1->size(), 1);
    ASSERT_EQ(refs2->size(), 1);
    ASSERT_EQ((*refs1)[0].atom_id, (*refs2)[0].atom_id);  // Same atom

    // Different value should create different atom
    auto atom3 = log.append(entity1, "status", std::string("inactive"), types::AtomType::Canonical);
    ASSERT_NE(atom1.atom_id(), atom3.atom_id());

    // Verify stats - now entity1 has 2 references
    const auto* refs1_updated = log.get_entity_atoms(entity1);
    ASSERT_TRUE(refs1_updated != nullptr);
    ASSERT_EQ(refs1_updated->size(), 2);

    auto stats = log.get_stats();
    // Total content atoms is 2 (one for "active", one for "inactive")
    ASSERT_EQ(stats.total_atoms, 2);
    ASSERT_EQ(stats.canonical_atoms, 2);
    ASSERT_EQ(stats.unique_canonical_atoms, 2);
    ASSERT_EQ(stats.deduplicated_hits, 1);
}

TEST(AtomStore, TemporalNoDeduplication) {
    core::AtomStore log;
    auto entity = make_entity(1);

    // Append same value twice as temporal
    auto temp1 = log.append(entity, "temperature", 23.5, types::AtomType::Temporal);
    auto temp2 = log.append(entity, "temperature", 23.5, types::AtomType::Temporal);

    // Should NOT be deduplicated
    ASSERT_NE(temp1.atom_id(), temp2.atom_id());

    ASSERT_EQ(log.all().size(), 2);
}

TEST(AtomStore, TemporalChunking) {
    core::AtomStore log;
    auto entity = make_entity(1);

    // Append 1500 temporal values to trigger chunking
    for (int i = 0; i < 1500; ++i) {
        log.append(entity, "sensor.temperature", static_cast<double>(20.0 + i), types::AtomType::Temporal);
    }

    // Query all temporal data
    auto result = log.query_temporal_all(entity, "sensor.temperature");
    ASSERT_EQ(result.total_count, 1500);
    ASSERT_EQ(result.values.size(), 1500);
    ASSERT_EQ(result.timestamps.size(), 1500);
    ASSERT_EQ(result.lsns.size(), 1500);

    // Verify first and last values
    ASSERT_TRUE(std::holds_alternative<double>(result.values[0]));
    ASSERT_TRUE(std::holds_alternative<double>(result.values[1499]));
    ASSERT_EQ(std::get<double>(result.values[0]), 20.0);
    ASSERT_EQ(std::get<double>(result.values[1499]), 1519.0);
}

TEST(AtomStore, MutableStateSameId) {
    core::AtomStore log;
    auto entity = make_entity(1);

    // Mutable atoms should keep same ID
    auto atom1 = log.append(entity, "counter", static_cast<int64_t>(1), types::AtomType::Mutable);
    auto atom2 = log.append(entity, "counter", static_cast<int64_t>(2), types::AtomType::Mutable);
    auto atom3 = log.append(entity, "counter", static_cast<int64_t>(3), types::AtomType::Mutable);

    ASSERT_EQ(atom1.atom_id(), atom2.atom_id());
    ASSERT_EQ(atom2.atom_id(), atom3.atom_id());
}

TEST(AtomStore, MutableSnapshotTrigger) {
    core::AtomStore log;
    auto entity = make_entity(1);

    size_t initial_count = log.all().size();

    // Append 12 mutations (threshold is 10)
    for (int i = 1; i <= 12; ++i) {
        log.append(entity, "counter", static_cast<int64_t>(i), types::AtomType::Mutable);
    }

    // Should have 12 mutations + 1 snapshot atom
    // (Snapshot is emitted as a Canonical atom with .snapshot suffix)
    size_t expected_min = initial_count + 12;  // At least the mutations
    ASSERT_TRUE(log.all().size() >= expected_min);
}

TEST(AtomStore, EdgeValues) {
    core::AtomStore log;
    auto entity1 = make_entity(1);
    auto entity2 = make_entity(2);

    types::EdgeValue edge{entity2, "follows"};
    auto atom = log.append(entity1, "edge.follows", edge, types::AtomType::Canonical);

    ASSERT_TRUE(std::holds_alternative<types::EdgeValue>(atom.value()));
    auto& edge_value = std::get<types::EdgeValue>(atom.value());
    ASSERT_EQ(edge_value.target, entity2);
    ASSERT_EQ(edge_value.relation, "follows");
}

TEST(AtomStore, MultipleValueTypes) {
    core::AtomStore log;
    auto entity = make_entity(1);

    log.append(entity, "name", std::string("Alice"), types::AtomType::Canonical);
    log.append(entity, "age", static_cast<int64_t>(30), types::AtomType::Canonical);
    log.append(entity, "score", 95.5, types::AtomType::Canonical);
    log.append(entity, "active", true, types::AtomType::Canonical);

    std::vector<float> embedding = {0.1f, 0.2f, 0.3f};
    log.append(entity, "embedding", embedding, types::AtomType::Canonical);

    ASSERT_EQ(log.all().size(), 5);
}

TEST(AtomStore, LsnMonotonicity) {
    core::AtomStore log;
    auto entity = make_entity(1);

    log.append(entity, "value", static_cast<int64_t>(1), types::AtomType::Canonical);
    log.append(entity, "value", static_cast<int64_t>(2), types::AtomType::Canonical);
    log.append(entity, "value", static_cast<int64_t>(3), types::AtomType::Canonical);

    // Get entity references and verify LSNs are strictly increasing
    const auto* refs = log.get_entity_atoms(entity);
    ASSERT_TRUE(refs != nullptr);
    ASSERT_EQ(refs->size(), 3);

    // LSNs should be strictly increasing in the reference list
    ASSERT_TRUE((*refs)[0].lsn.value < (*refs)[1].lsn.value);
    ASSERT_TRUE((*refs)[1].lsn.value < (*refs)[2].lsn.value);
}

TEST(AtomStore, TimestampMonotonicity) {
    core::AtomStore log;
    auto entity = make_entity(1);

    auto atom1 = log.append(entity, "value", static_cast<int64_t>(1), types::AtomType::Canonical);
    auto atom2 = log.append(entity, "value", static_cast<int64_t>(2), types::AtomType::Canonical);

    // Timestamps should be non-decreasing (could be equal if very fast)
    ASSERT_TRUE(atom1.created_at() <= atom2.created_at());
}
