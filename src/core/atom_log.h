// atom_log.h
#pragma once
#include "atom.h"
#include "../types/hash_utils.h"
#include <vector>
#include <chrono>
#include <cstring>
#include <algorithm>
#include <unordered_map>
#include <optional>

namespace gtaf::core {

// Hash function for AtomId to use in unordered_map
struct AtomIdHash {
    std::size_t operator()(const types::AtomId& id) const noexcept {
        // Use first 8 bytes as hash
        uint64_t hash;
        std::memcpy(&hash, id.bytes.data(), sizeof(hash));
        return static_cast<std::size_t>(hash);
    }
};

class AtomLog {
public:
    /**
     * @brief Append an atom to the log with proper classification handling
     *
     * - Canonical atoms: content-addressed with global deduplication
     * - Temporal atoms: sequential IDs, no deduplication
     * - Mutable atoms: sequential IDs, delta logging (TODO)
     *
     * @return The created (or existing) Atom
     */
    Atom append(
        types::EntityId entity,
        std::string tag,
        types::AtomValue value,
        types::AtomType classification = types::AtomType::Canonical
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

    const std::vector<Atom>& all() const { return m_atoms; }

    /**
     * @brief Get deduplication statistics
     */
    struct Stats {
        size_t total_atoms = 0;
        size_t canonical_atoms = 0;
        size_t deduplicated_hits = 0;
        size_t unique_canonical_atoms = 0;
    };

    Stats get_stats() const {
        Stats stats;
        stats.total_atoms = m_atoms.size();
        stats.canonical_atoms = m_canonical_atom_count;
        stats.deduplicated_hits = m_dedup_hits;
        stats.unique_canonical_atoms = m_canonical_dedup_map.size();
        return stats;
    }

private:
    /**
     * @brief Append a Canonical atom (immutable, content-addressed, deduplicated)
     */
    Atom append_canonical(
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

    /**
     * @brief Append a Temporal atom (time-series, no deduplication)
     */
    Atom append_temporal(
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

    /**
     * @brief Append a Mutable atom (counters, aggregates, delta-logged)
     */
    Atom append_mutable(
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

    /**
     * @brief Generate sequential AtomId for non-canonical atoms
     */
    types::AtomId generate_sequential_id() {
        types::AtomId atom_id;
        std::fill(atom_id.bytes.begin(), atom_id.bytes.end(), 0);
        uint64_t id_val = ++m_next_atom_id;
        std::memcpy(atom_id.bytes.data(), &id_val, sizeof(id_val));
        return atom_id;
    }

    /**
     * @brief Get current timestamp in microseconds
     */
    types::Timestamp get_current_timestamp() const {
        return std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
    }

    // Sequential ID counter (for Temporal and Mutable atoms)
    uint64_t m_next_atom_id = 0;

    // Log sequence number (for all atoms)
    uint64_t m_next_lsn = 0;

    // Append-only atom storage
    std::vector<Atom> m_atoms;

    // Deduplication map: hash -> index in m_atoms
    // Only used for Canonical atoms
    std::unordered_map<types::AtomId, size_t, AtomIdHash> m_canonical_dedup_map;

    // Statistics
    size_t m_canonical_atom_count = 0;
    size_t m_dedup_hits = 0;
};

} // namespace gtaf::core
