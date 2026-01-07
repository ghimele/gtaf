#include "atom_log.h"
#include "../types/hash_utils.h"
#include <chrono>
#include <algorithm>

namespace gtaf::core {

// ---- AtomLog Implementation ----

Atom AtomLog::append(
    types::EntityId entity,
    std::string tag,
    types::AtomValue value,
    types::AtomType classification
) {
    // Route to appropriate write path based on classification
    switch (classification) {
        case types::AtomType::Canonical:
            return append_canonical(entity, std::move(tag), std::move(value));
        case types::AtomType::Temporal:
            return append_temporal(entity, std::move(tag), std::move(value));
        case types::AtomType::Mutable:
            return append_mutable(entity, std::move(tag), std::move(value));
    }
    // Should never reach here, but satisfy compiler
    return append_temporal(entity, std::move(tag), std::move(value));
}

const std::vector<Atom>& AtomLog::all() const {
    return m_atoms;
}

AtomLog::Stats AtomLog::get_stats() const {
    Stats stats;
    stats.total_atoms = m_atoms.size();
    stats.canonical_atoms = m_canonical_atom_count;
    stats.deduplicated_hits = m_dedup_hits;
    stats.unique_canonical_atoms = m_canonical_dedup_map.size();
    return stats;
}

Atom AtomLog::append_canonical(
    types::EntityId entity,
    std::string tag,
    types::AtomValue value
) {
    // Compute content-based hash
    types::AtomId atom_id = types::compute_content_hash(tag, value);

    // Check for existing atom with same hash (deduplication)
    if (auto it = m_canonical_dedup_map.find(atom_id); it != m_canonical_dedup_map.end()) {
        // Atom already exists - return existing atom
        ++m_dedup_hits;
        return m_atoms[it->second];
    }

    // New canonical atom - create and store
    types::LogSequenceNumber lsn{++m_next_lsn};
    types::Timestamp now = get_current_timestamp();

    Atom atom(
        atom_id,
        entity,
        types::AtomType::Canonical,
        std::move(tag),
        std::move(value),
        lsn,
        now
    );

    // Store in log and dedup map
    size_t index = m_atoms.size();
    m_atoms.push_back(atom);
    m_canonical_dedup_map[atom_id] = index;
    ++m_canonical_atom_count;

    return atom;
}

Atom AtomLog::append_temporal(
    types::EntityId entity,
    std::string tag,
    types::AtomValue value
) {
    // Generate sequential ID for temporal atoms
    types::AtomId atom_id = generate_sequential_id();

    types::LogSequenceNumber lsn{++m_next_lsn};
    types::Timestamp now = get_current_timestamp();

    Atom atom(
        atom_id,
        entity,
        types::AtomType::Temporal,
        std::move(tag),
        std::move(value),
        lsn,
        now
    );

    m_atoms.push_back(atom);
    return atom;
}

Atom AtomLog::append_mutable(
    types::EntityId entity,
    std::string tag,
    types::AtomValue value
) {
    // Generate sequential ID for mutable atoms
    types::AtomId atom_id = generate_sequential_id();

    types::LogSequenceNumber lsn{++m_next_lsn};
    types::Timestamp now = get_current_timestamp();

    Atom atom(
        atom_id,
        entity,
        types::AtomType::Mutable,
        std::move(tag),
        std::move(value),
        lsn,
        now
    );

    m_atoms.push_back(atom);
    // TODO: Implement delta logging and snapshot emission
    return atom;
}

types::AtomId AtomLog::generate_sequential_id() {
    types::AtomId atom_id;
    std::fill(atom_id.bytes.begin(), atom_id.bytes.end(), 0);
    uint64_t id_val = ++m_next_atom_id;
    std::memcpy(atom_id.bytes.data(), &id_val, sizeof(id_val));
    return atom_id;
}

types::Timestamp AtomLog::get_current_timestamp() const {
    return std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

} // namespace gtaf::core
