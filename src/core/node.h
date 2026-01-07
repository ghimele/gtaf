#pragma once

#include "../types/types.h"
#include <unordered_map>
#include <vector>
#include <optional>

namespace gtaf::core {

class Node final {
public:
    explicit Node(types::EntityId entity_id)
        : m_entity_id(entity_id) {}

    // ---- Identity ----
    [[nodiscard]] const types::EntityId& entity_id() const noexcept {
        return m_entity_id;
    }

    // ---- Projection update ----
    // Apply an atom that belongs to this entity
    void apply(
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

    // ---- Queries ----

    [[nodiscard]] std::optional<types::AtomId>
    latest_atom(const std::string& type_tag) const {
        if (auto it = m_latest_by_tag.find(type_tag); it != m_latest_by_tag.end()) {
            return it->second->atom_id;
        }
        return std::nullopt;
    }

    [[nodiscard]] const std::vector<std::pair<types::AtomId, types::LogSequenceNumber>>&
    history() const noexcept {
        return m_history;
    }

private:
    struct Entry {
        types::AtomId atom_id;
        types::LogSequenceNumber lsn;
    };

    // ---- Identity ----
    types::EntityId m_entity_id;

    // ---- Derived state ----
    std::unordered_map<std::string, std::optional<Entry>> m_latest_by_tag;
    std::vector<std::pair<types::AtomId, types::LogSequenceNumber>> m_history;
};

} // namespace gtaf::core
