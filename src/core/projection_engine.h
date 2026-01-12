// projection_engine.h
#pragma once
#include "atom_log.h"
#include "node.h"
#include <unordered_map>
#include <vector>
#include <cstring>

namespace gtaf::core {

// EntityIdHash is now defined in atom_log.h

/**
 * @brief Engine for rebuilding Node projections from the atom log
 *
 * The ProjectionEngine iterates through the log and reconstructs
 * entity state by filtering and applying relevant atoms.
 */
class ProjectionEngine {
public:
    /**
     * @brief Construct a ProjectionEngine for a given log
     *
     * @param log Reference to the atom log (must outlive this engine)
     */
    explicit ProjectionEngine(const AtomLog& log);

    /**
     * @brief Rebuild a Node projection for a specific entity
     *
     * Scans the entire log, filters atoms by entity_id, and applies them
     * in sequence to build the current state.
     *
     * @param entity The entity to rebuild
     * @return Fully reconstructed Node
     */
    Node rebuild(types::EntityId entity) const;

    /**
     * @brief Get all unique entity IDs present in the log
     *
     * Scans the log and collects all unique entity IDs.
     *
     * @return Vector of unique entity IDs
     */
    std::vector<types::EntityId> get_all_entities() const;

    /**
     * @brief Rebuild all nodes for all entities in the log
     *
     * Efficiently builds a complete projection by scanning the log once
     * and distributing atoms to their respective nodes.
     *
     * @return Map of entity_id -> Node for all entities
     */
    std::unordered_map<types::EntityId, Node, EntityIdHash> rebuild_all() const;

    /**
     * @brief Stream-process all nodes with a callback function
     *
     * Efficiently builds nodes in batches and processes them via callback,
     * keeping memory usage bounded. This combines the performance of rebuild_all
     * (single pass through atom log) with the memory efficiency of on-demand rebuilding.
     *
     * @param callback Function called for each (entity_id, node) pair
     * @param batch_size Number of nodes to build before invoking callbacks (default: 1000)
     *
     * Example:
     *   projector.rebuild_all_streaming([&](const EntityId& id, const Node& node) {
     *       // Process node here - it will be freed after callback returns
     *       if (matches_criteria(node)) { count++; }
     *   });
     */
    template<typename Callback>
    void rebuild_all_streaming(Callback callback, size_t batch_size = 1000) const;

private:
    const AtomLog& m_log;
};

// Template implementation (must be in header)
template<typename Callback>
void ProjectionEngine::rebuild_all_streaming(Callback callback, size_t batch_size) const {
    // Get all entities from the reference layer
    auto entities = m_log.get_all_entities();

    // Process entities in batches to control memory usage
    std::unordered_map<types::EntityId, Node, EntityIdHash> batch_nodes;
    batch_nodes.reserve(batch_size);

    size_t processed = 0;
    for (const auto& entity : entities) {
        // Build node and add to current batch
        batch_nodes.emplace(entity, rebuild(entity));

        // When batch is full, process all nodes and clear
        if (batch_nodes.size() >= batch_size) {
            for (const auto& [ent_id, node] : batch_nodes) {
                callback(ent_id, node);
            }
            batch_nodes.clear();
            processed += batch_size;
        }
    }

    // Process remaining nodes in final batch
    for (const auto& [ent_id, node] : batch_nodes) {
        callback(ent_id, node);
    }
}

} // namespace gtaf::core
