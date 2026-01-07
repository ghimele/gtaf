#pragma once

#include "../types/types.h"
#include <unordered_map>
#include <vector>
#include <optional>

namespace gtaf::core {

/**
 * @brief Represents the projected state of a single entity
 *
 * A Node is a derived, mutable view rebuilt from the atom log.
 * It tracks the latest atom for each type_tag and maintains full history.
 */
class Node final {
public:
    /**
     * @brief Construct a Node for a given entity
     */
    explicit Node(types::EntityId entity_id);

    /**
     * @brief Get the entity ID this Node represents
     */
    [[nodiscard]] const types::EntityId& entity_id() const noexcept;

    /**
     * @brief Apply an atom that belongs to this entity
     *
     * Updates the latest atom for the type_tag if the LSN is newer.
     * Always appends to history.
     */
    void apply(
        const types::AtomId& atom_id,
        const std::string& type_tag,
        types::LogSequenceNumber lsn
    );

    /**
     * @brief Query the latest atom for a given type_tag
     *
     * @return AtomId if found, nullopt otherwise
     */
    [[nodiscard]] std::optional<types::AtomId>
    latest_atom(const std::string& type_tag) const;

    /**
     * @brief Get the complete history of atoms applied to this Node
     */
    [[nodiscard]] const std::vector<std::pair<types::AtomId, types::LogSequenceNumber>>&
    history() const noexcept;

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
