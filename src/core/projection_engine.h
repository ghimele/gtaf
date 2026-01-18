// projection_engine.h
#pragma once
#include "atom_store.h"
#include "node.h"
#include <unordered_map>
#include <vector>
#include <cstring>

namespace gtaf::core {

// EntityIdHash is now defined in atom_store.h

/**
 * @brief Engine for rebuilding Node projections from the atom store
 *
 * The ProjectionEngine iterates through the store and reconstructs
 * entity state by filtering and applying relevant atoms.
 */
class ProjectionEngine {
public:
    /**
     * @brief Construct a ProjectionEngine for a given store
     *
     * @param store Reference to the atom store (must outlive this engine)
     */
    explicit ProjectionEngine(const AtomStore& store);

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
    const AtomStore& m_store;
};

// Template implementation (must be in header)
template<typename Callback>
void ProjectionEngine::rebuild_all_streaming(Callback callback, [[maybe_unused]] size_t batch_size) const {
    // Get all entities from the reference layer
    auto entities = m_store.get_all_entities();

    // Process each entity directly - no batching needed since we're streaming
    for (const auto& entity : entities) {
        // Build node inline and pass to callback immediately
        Node node(entity);

        // Get atom references directly (no copy, just pointer)
        const auto* refs = m_store.get_entity_atoms(entity);
        if (refs) {
            for (const auto& ref : *refs) {
                const Atom* atom = m_store.get_atom(ref.atom_id);
                if (atom) {
                    node.apply(atom->atom_id(), atom->type_tag(), atom->value(), ref.lsn);
                }
            }
        }

        callback(entity, node);
    }
}

} // namespace gtaf::core
