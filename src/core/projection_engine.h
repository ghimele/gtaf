// projection_engine.h
#pragma once
#include "atom_log.h"
#include "node.h"

namespace gtaf::core {

class ProjectionEngine {
public:
    explicit ProjectionEngine(const AtomLog& log) : m_log(log) {}

    Node rebuild(types::EntityId entity) const {
        Node node(entity);
        for (const auto& atom : m_log.all()) {
            if (atom.entity_id() == entity) {
                node.apply(atom.atom_id(), atom.type_tag(), atom.lsn());
            }
        }
        return node;
    }

private:
    const AtomLog& m_log;
};

} // namespace gtaf::core
