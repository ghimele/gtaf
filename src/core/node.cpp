#include "node.h"

namespace gtaf::core {

// ---- Node Implementation ----

Node::Node(types::EntityId entity_id)
    : m_entity_id(entity_id) {}

const types::EntityId& Node::entity_id() const noexcept {
    return m_entity_id;
}

void Node::apply(
    const types::AtomId& atom_id,
    const std::string& type_tag,
    types::LogSequenceNumber lsn
) {
    auto& slot = m_latest_by_tag[type_tag];
    if (!slot || lsn > slot->lsn) {
        slot = Entry{ atom_id, lsn };
    }

    m_history.emplace_back(atom_id, lsn);
}

std::optional<types::AtomId> Node::latest_atom(const std::string& type_tag) const {
    if (auto it = m_latest_by_tag.find(type_tag); it != m_latest_by_tag.end()) {
        return it->second->atom_id;
    }
    return std::nullopt;
}

const std::vector<std::pair<types::AtomId, types::LogSequenceNumber>>&
Node::history() const noexcept {
    return m_history;
}

} // namespace gtaf::core
