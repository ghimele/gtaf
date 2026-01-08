// projection_engine.h
#pragma once
#include "atom_log.h"
#include "node.h"
#include <unordered_map>
#include <vector>
#include <cstring>

namespace gtaf::core {

// Hash function for EntityId to use in unordered_map
struct EntityIdHash {
    std::size_t operator()(const types::EntityId& id) const noexcept {
        // Use first 8 bytes as hash
        uint64_t hash;
        std::memcpy(&hash, id.bytes.data(), sizeof(hash));
        return static_cast<std::size_t>(hash);
    }
};

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

private:
    const AtomLog& m_log;
};

} // namespace gtaf::core
