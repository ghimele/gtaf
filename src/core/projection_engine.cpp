#include "projection_engine.h"
#include <unordered_set>

namespace gtaf::core {

// ---- ProjectionEngine Implementation ----

ProjectionEngine::ProjectionEngine(const AtomStore& store)
    : m_store(store) {}

Node ProjectionEngine::rebuild(types::EntityId entity) const {
    Node node(entity);

    // Get all atom references for this entity (already sorted by LSN)
    const auto* refs = m_store.get_entity_atoms(entity);
    if (!refs) {
        return node;  // No atoms for this entity
    }

    // Apply each atom in chronological order
    for (const auto& ref : *refs) {
        const Atom* atom = m_store.get_atom(ref.atom_id);
        if (atom) {
            node.apply(atom->atom_id(), atom->type_tag(), atom->value(), ref.lsn);
        }
    }

    return node;
}

std::vector<types::EntityId> ProjectionEngine::get_all_entities() const {
    return m_store.get_all_entities();
}

std::unordered_map<types::EntityId, Node, EntityIdHash> ProjectionEngine::rebuild_all() const {
    std::unordered_map<types::EntityId, Node, EntityIdHash> nodes;

    // Get all entities from the reference layer
    auto entities = m_store.get_all_entities();

    // Rebuild each entity
    for (const auto& entity : entities) {
        nodes.emplace(entity, rebuild(entity));
    }

    return nodes;
}

// Template implementation must be in header or explicitly instantiated
// For now, we'll move this to the header as an inline definition

} // namespace gtaf::core
