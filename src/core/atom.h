#pragma once

#include "../types/types.h"

namespace gtaf::core {

class Atom final {
public:
    Atom(
        types::AtomId atom_id,
        types::AtomType classification,
        std::string type_tag,
        types::AtomValue value,
        types::Timestamp created_at,
        types::TransactionId tx_id = {},
        uint32_t flags = 0
    )
        : m_atom_id(atom_id),
          m_classification(classification),
          m_type_tag(std::move(type_tag)),
          m_value(std::move(value)),
          m_created_at(created_at),
          m_tx_id(tx_id),
          m_flags(flags)
    {}

    // ---- Identity ----
    [[nodiscard]] types::AtomId atom_id() const noexcept { return m_atom_id; }
    // entity_id() removed - use AtomLog reference index instead

    // ---- Classification ----
    [[nodiscard]] types::AtomType classification() const noexcept { return m_classification; }
    [[nodiscard]] const std::string& type_tag() const noexcept { return m_type_tag; }

    // ---- Value ----
    [[nodiscard]] const types::AtomValue& value() const noexcept { return m_value; }

    // ---- Append-only metadata ----
    // Note: LSN is now tracked per-entity in AtomLog reference layer
    [[nodiscard]] types::Timestamp created_at() const noexcept { return m_created_at; }
    [[nodiscard]] types::TransactionId tx_id() const noexcept { return m_tx_id; }
    [[nodiscard]] uint32_t flags() const noexcept { return m_flags; }

    // ---- Convenience helpers ----
    [[nodiscard]] constexpr bool is_canonical() const noexcept {
        return m_classification == types::AtomType::Canonical;
    }

    [[nodiscard]] constexpr bool is_temporal() const noexcept {
        return m_classification == types::AtomType::Temporal;
    }

    [[nodiscard]] constexpr bool is_mutable() const noexcept {
        return m_classification == types::AtomType::Mutable;
    }

private:
    // ---- Identity ----
    types::AtomId   m_atom_id;
    // entity_id removed - tracked in AtomLog reference layer

    // ---- Classification ----
    types::AtomType m_classification;
    std::string     m_type_tag;

    // ---- Value ----
    types::AtomValue m_value;

    // ---- Append-only metadata ----
    // lsn removed - tracked per-entity in AtomLog reference layer
    types::Timestamp         m_created_at;
    types::TransactionId     m_tx_id;
    uint32_t                 m_flags;
};

} // namespace gtaf::core

/*
EntityId: 
    Identifies a logical entity
    Stable across time
    Can be referenced by:
        Atoms
        Nodes
        Edges
        Projections
        External systems

Has no behavior
Is not an object
Is not mutable
It is a coordinate in the data model.

AtomId:

Identifies a single append-only record
Never reused
Never shared
Exists exactly once in the log
Is not referenced for domain modeling
Used for:
    deduplication
    replay
    replication
    idempotency

It is a coordinate in the storage model.
*/