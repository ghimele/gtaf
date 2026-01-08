#include "projection_engine.h"
#include <unordered_set>

namespace gtaf::core {

// ---- ProjectionEngine Implementation ----

ProjectionEngine::ProjectionEngine(const AtomLog& log)
    : m_log(log) {}

Node ProjectionEngine::rebuild(types::EntityId entity) const {
    Node node(entity);
    for (const auto& atom : m_log.all()) {
        if (atom.entity_id() == entity) {
            node.apply(atom.atom_id(), atom.type_tag(), atom.value(), atom.lsn());
        }
    }
    return node;
}

std::vector<types::EntityId> ProjectionEngine::get_all_entities() const {
    // Use unordered_set to collect unique entity IDs
    std::unordered_set<types::EntityId, EntityIdHash> unique_entities;

    for (const auto& atom : m_log.all()) {
        unique_entities.insert(atom.entity_id());
    }

    // Convert to vector
    std::vector<types::EntityId> result;
    result.reserve(unique_entities.size());
    for (const auto& entity : unique_entities) {
        result.push_back(entity);
    }

    return result;
}

std::unordered_map<types::EntityId, Node, EntityIdHash> ProjectionEngine::rebuild_all() const {
    std::unordered_map<types::EntityId, Node, EntityIdHash> nodes;

    // Single pass through the log, distributing atoms to nodes
    for (const auto& atom : m_log.all()) {
        auto entity_id = atom.entity_id();

        // Get or create node for this entity
        auto it = nodes.find(entity_id);
        if (it == nodes.end()) {
            // Create new node
            auto [new_it, inserted] = nodes.emplace(entity_id, Node(entity_id));
            it = new_it;
        }

        // Apply atom to node
        it->second.apply(atom.atom_id(), atom.type_tag(), atom.value(), atom.lsn());
    }

    return nodes;
}

} // namespace gtaf::core
