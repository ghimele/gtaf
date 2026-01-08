#include "test_framework.h"
#include "../core/atom_log.h"
#include "../core/projection_engine.h"
#include <algorithm>

using namespace gtaf;
using namespace gtaf::test;

// Helper to create test EntityIds
types::EntityId make_entity_node(uint8_t id) {
    types::EntityId entity{};
    std::fill(entity.bytes.begin(), entity.bytes.end(), 0);
    entity.bytes[0] = id;
    return entity;
}

TEST(Node, BasicProjection) {
    core::AtomLog log;
    auto entity = make_entity_node(1);

    log.append(entity, "name", std::string("Alice"), types::AtomType::Canonical);
    log.append(entity, "age", static_cast<int64_t>(30), types::AtomType::Canonical);

    core::ProjectionEngine projector(log);
    auto node = projector.rebuild(entity);

    // Verify values are projected
    auto name = node.get("name");
    ASSERT_TRUE(name.has_value());
    ASSERT_TRUE(std::holds_alternative<std::string>(*name));
    ASSERT_EQ(std::get<std::string>(*name), "Alice");

    auto age = node.get("age");
    ASSERT_TRUE(age.has_value());
    ASSERT_TRUE(std::holds_alternative<int64_t>(*age));
    ASSERT_EQ(std::get<int64_t>(*age), 30);
}

TEST(Node, LatestValueWins) {
    core::AtomLog log;
    auto entity = make_entity_node(1);

    log.append(entity, "status", std::string("active"), types::AtomType::Canonical);
    log.append(entity, "status", std::string("inactive"), types::AtomType::Canonical);
    log.append(entity, "status", std::string("suspended"), types::AtomType::Canonical);

    core::ProjectionEngine projector(log);
    auto node = projector.rebuild(entity);

    auto status = node.get("status");
    ASSERT_TRUE(status.has_value());
    ASSERT_EQ(std::get<std::string>(*status), "suspended");
}

TEST(Node, GetAllProperties) {
    core::AtomLog log;
    auto entity = make_entity_node(1);

    log.append(entity, "name", std::string("Bob"), types::AtomType::Canonical);
    log.append(entity, "age", static_cast<int64_t>(25), types::AtomType::Canonical);
    log.append(entity, "active", true, types::AtomType::Canonical);

    core::ProjectionEngine projector(log);
    auto node = projector.rebuild(entity);

    auto all = node.get_all();
    ASSERT_EQ(all.size(), 3);
    ASSERT_TRUE(all.find("name") != all.end());
    ASSERT_TRUE(all.find("age") != all.end());
    ASSERT_TRUE(all.find("active") != all.end());
}

TEST(Node, MultipleEntities) {
    core::AtomLog log;
    auto entity1 = make_entity_node(1);
    auto entity2 = make_entity_node(2);

    log.append(entity1, "name", std::string("Alice"), types::AtomType::Canonical);
    log.append(entity2, "name", std::string("Bob"), types::AtomType::Canonical);

    core::ProjectionEngine projector(log);
    auto node1 = projector.rebuild(entity1);
    auto node2 = projector.rebuild(entity2);

    auto name1 = node1.get("name");
    auto name2 = node2.get("name");

    ASSERT_TRUE(name1.has_value());
    ASSERT_TRUE(name2.has_value());
    ASSERT_EQ(std::get<std::string>(*name1), "Alice");
    ASSERT_EQ(std::get<std::string>(*name2), "Bob");
}

TEST(Node, History) {
    core::AtomLog log;
    auto entity = make_entity_node(1);

    log.append(entity, "value", static_cast<int64_t>(1), types::AtomType::Canonical);
    log.append(entity, "value", static_cast<int64_t>(2), types::AtomType::Canonical);
    log.append(entity, "value", static_cast<int64_t>(3), types::AtomType::Canonical);

    core::ProjectionEngine projector(log);
    auto node = projector.rebuild(entity);

    const auto& history = node.history();
    ASSERT_EQ(history.size(), 3);

    // History should be in order
    ASSERT_TRUE(history[0].second.value < history[1].second.value);
    ASSERT_TRUE(history[1].second.value < history[2].second.value);
}

TEST(Node, EmptyEntity) {
    core::AtomLog log;
    auto entity = make_entity_node(1);

    // Append to different entity
    log.append(make_entity_node(2), "name", std::string("Alice"), types::AtomType::Canonical);

    core::ProjectionEngine projector(log);
    auto node = projector.rebuild(entity);

    // Should be empty
    ASSERT_EQ(node.history().size(), 0);
    ASSERT_EQ(node.get_all().size(), 0);
}

TEST(Node, MutablePropertyUpdate) {
    core::AtomLog log;
    auto entity = make_entity_node(1);

    log.append(entity, "counter", static_cast<int64_t>(1), types::AtomType::Mutable);
    log.append(entity, "counter", static_cast<int64_t>(5), types::AtomType::Mutable);
    log.append(entity, "counter", static_cast<int64_t>(10), types::AtomType::Mutable);

    core::ProjectionEngine projector(log);
    auto node = projector.rebuild(entity);

    auto counter = node.get("counter");
    ASSERT_TRUE(counter.has_value());
    ASSERT_EQ(std::get<int64_t>(*counter), 10);
}

TEST(Node, NonExistentProperty) {
    core::AtomLog log;
    auto entity = make_entity_node(1);

    log.append(entity, "name", std::string("Alice"), types::AtomType::Canonical);

    core::ProjectionEngine projector(log);
    auto node = projector.rebuild(entity);

    auto missing = node.get("nonexistent");
    ASSERT_FALSE(missing.has_value());
}

TEST(Node, LatestAtom) {
    core::AtomLog log;
    auto entity = make_entity_node(1);

    auto atom1 = log.append(entity, "value", static_cast<int64_t>(1), types::AtomType::Canonical);
    auto atom2 = log.append(entity, "value", static_cast<int64_t>(2), types::AtomType::Canonical);

    core::ProjectionEngine projector(log);
    auto node = projector.rebuild(entity);

    auto latest = node.latest_atom("value");
    ASSERT_TRUE(latest.has_value());
    ASSERT_EQ(*latest, atom2.atom_id());
}

TEST(ProjectionEngine, GetAllEntities) {
    core::AtomLog log;
    auto entity1 = make_entity_node(1);
    auto entity2 = make_entity_node(2);
    auto entity3 = make_entity_node(3);

    log.append(entity1, "name", std::string("Alice"), types::AtomType::Canonical);
    log.append(entity2, "name", std::string("Bob"), types::AtomType::Canonical);
    log.append(entity3, "name", std::string("Charlie"), types::AtomType::Canonical);
    log.append(entity1, "age", static_cast<int64_t>(30), types::AtomType::Canonical);

    core::ProjectionEngine projector(log);
    auto entities = projector.get_all_entities();

    // Should have 3 unique entities
    ASSERT_EQ(entities.size(), 3);
}

TEST(ProjectionEngine, RebuildAll) {
    core::AtomLog log;
    auto entity1 = make_entity_node(1);
    auto entity2 = make_entity_node(2);

    log.append(entity1, "name", std::string("Alice"), types::AtomType::Canonical);
    log.append(entity1, "age", static_cast<int64_t>(30), types::AtomType::Canonical);
    log.append(entity2, "name", std::string("Bob"), types::AtomType::Canonical);
    log.append(entity2, "age", static_cast<int64_t>(25), types::AtomType::Canonical);

    core::ProjectionEngine projector(log);
    auto all_nodes = projector.rebuild_all();

    // Should have 2 nodes
    ASSERT_EQ(all_nodes.size(), 2);

    // Verify entity1 node
    auto it1 = all_nodes.find(entity1);
    ASSERT_TRUE(it1 != all_nodes.end());
    auto name1 = it1->second.get("name");
    ASSERT_TRUE(name1.has_value());
    ASSERT_EQ(std::get<std::string>(*name1), "Alice");

    // Verify entity2 node
    auto it2 = all_nodes.find(entity2);
    ASSERT_TRUE(it2 != all_nodes.end());
    auto name2 = it2->second.get("name");
    ASSERT_TRUE(name2.has_value());
    ASSERT_EQ(std::get<std::string>(*name2), "Bob");
}

TEST(ProjectionEngine, RebuildAllEfficiency) {
    core::AtomLog log;

    // Create 50 entities with unique properties
    for (int i = 1; i <= 50; ++i) {
        auto entity = make_entity_node(static_cast<uint8_t>(i));
        for (int j = 0; j < 10; ++j) {
            // Use entity-specific values to avoid deduplication affecting entity count
            log.append(entity, "prop" + std::to_string(j),
                      static_cast<int64_t>(i * 1000 + j), types::AtomType::Temporal);
        }
    }

    core::ProjectionEngine projector(log);
    auto all_nodes = projector.rebuild_all();

    // Should have 50 nodes
    ASSERT_EQ(all_nodes.size(), 50);

    // Each node should have 10 properties
    for (const auto& [entity_id, node] : all_nodes) {
        ASSERT_EQ(node.get_all().size(), 10);
    }
}
