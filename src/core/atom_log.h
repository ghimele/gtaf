// atom_log.h
#pragma once
#include "atom.h"
#include "temporal_chunk.h"
#include <vector>
#include <unordered_map>
#include <cstddef>
#include <cstring>

namespace gtaf::core {

// Hash function for AtomId to use in unordered_map
// Must be defined here (not forward declared) because it's used as a template parameter
struct AtomIdHash {
    std::size_t operator()(const types::AtomId& id) const noexcept {
        // Use first 8 bytes as hash
        uint64_t hash;
        std::memcpy(&hash, id.bytes.data(), sizeof(hash));
        return static_cast<std::size_t>(hash);
    }
};

// Key for tracking temporal chunks by (entity, tag) pair
struct TemporalKey {
    types::EntityId entity_id;
    std::string tag;

    bool operator==(const TemporalKey& other) const = default;
};

// Hash function for TemporalKey
struct TemporalKeyHash {
    std::size_t operator()(const TemporalKey& key) const noexcept {
        // Combine entity hash and tag hash
        std::hash<std::string> tag_hash;
        uint64_t entity_hash = 0;
        std::memcpy(&entity_hash, key.entity_id.bytes.data(), 8);
        return entity_hash ^ tag_hash(key.tag);
    }
};

/**
 * @brief Append-only log for storing Atoms with classification-aware write paths
 *
 * The AtomLog implements the core persistence mechanism for GTAF, routing writes
 * to appropriate handlers based on Atom classification:
 * - Canonical: content-addressed with global deduplication
 * - Temporal: sequential IDs, optimized for time-series
 * - Mutable: sequential IDs with delta logging (TODO)
 */
class AtomLog {
public:
    /**
     * @brief Append an atom to the log with proper classification handling
     *
     * @param entity The entity this atom belongs to
     * @param tag The semantic type tag (e.g., "user.name")
     * @param value The atom value
     * @param classification The atom class (Canonical, Temporal, or Mutable)
     * @return The created (or existing) Atom
     */
    Atom append(
        types::EntityId entity,
        std::string tag,
        types::AtomValue value,
        types::AtomType classification = types::AtomType::Canonical
    );

    /**
     * @brief Get all atoms in the log
     */
    const std::vector<Atom>& all() const;

    /**
     * @brief Deduplication and storage statistics
     */
    struct Stats {
        size_t total_atoms = 0;
        size_t canonical_atoms = 0;
        size_t deduplicated_hits = 0;
        size_t unique_canonical_atoms = 0;
    };

    /**
     * @brief Get deduplication statistics
     */
    Stats get_stats() const;

private:
    /**
     * @brief Append a Canonical atom (immutable, content-addressed, deduplicated)
     */
    Atom append_canonical(
        types::EntityId entity,
        std::string tag,
        types::AtomValue value
    );

    /**
     * @brief Append a Temporal atom (time-series, no deduplication)
     */
    Atom append_temporal(
        types::EntityId entity,
        std::string tag,
        types::AtomValue value
    );

    /**
     * @brief Append a Mutable atom (counters, aggregates, delta-logged)
     */
    Atom append_mutable(
        types::EntityId entity,
        std::string tag,
        types::AtomValue value
    );

    /**
     * @brief Generate sequential AtomId for non-canonical atoms
     */
    types::AtomId generate_sequential_id();

    /**
     * @brief Get current timestamp in microseconds
     */
    types::Timestamp get_current_timestamp() const;

    /**
     * @brief Get or create active chunk for a (entity, tag) stream
     */
    TemporalChunk& get_or_create_active_chunk(const TemporalKey& key);

    /**
     * @brief Seal current chunk and prepare for next chunk
     */
    void seal_and_rotate_chunk(const TemporalKey& key);

    // Sequential ID counter (for Temporal and Mutable atoms)
    uint64_t m_next_atom_id = 0;

    // Log sequence number (for all atoms)
    uint64_t m_next_lsn = 0;

    // Append-only atom storage
    std::vector<Atom> m_atoms;

    // Deduplication map: hash -> index in m_atoms
    // Only used for Canonical atoms
    std::unordered_map<types::AtomId, size_t, AtomIdHash> m_canonical_dedup_map;

    // --- Temporal Chunk Management ---

    // Active chunks (one per entity+tag stream)
    std::unordered_map<TemporalKey, TemporalChunk, TemporalKeyHash> m_active_chunks;

    // Sealed chunks for history queries
    std::unordered_map<types::ChunkId, TemporalChunk> m_sealed_chunks;

    // Chunk ID counter per stream
    std::unordered_map<TemporalKey, types::ChunkId, TemporalKeyHash> m_next_chunk_id;

    // Configuration
    size_t m_chunk_size_threshold = 1000;  // Values per chunk

    // Statistics
    size_t m_canonical_atom_count = 0;
    size_t m_dedup_hits = 0;
};

} // namespace gtaf::core
