// projection_engine.h
#pragma once
#include "atom_log.h"
#include "node.h"

namespace gtaf::core {

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

private:
    const AtomLog& m_log;
};

} // namespace gtaf::core
