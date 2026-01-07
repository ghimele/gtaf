#include "projection_engine.h"

namespace gtaf::core {

// ---- ProjectionEngine Implementation ----

ProjectionEngine::ProjectionEngine(const AtomLog& log)
    : m_log(log) {}

Node ProjectionEngine::rebuild(types::EntityId entity) const {
    Node node(entity);
    for (const auto& atom : m_log.all()) {
        if (atom.entity_id() == entity) {
            node.apply(atom.atom_id(), atom.type_tag(), atom.lsn());
        }
    }
    return node;
}

} // namespace gtaf::core
