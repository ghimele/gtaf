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
    types::LogSequenceNumber lsn{++m_next_lsn};
    types::Timestamp now = get_current_timestamp();

    // Create key for this temporal stream
    TemporalKey key{entity, tag};

    // Get or create active chunk for this stream
    TemporalChunk& chunk = get_or_create_active_chunk(key);

    // Append value to chunk
    chunk.append(value, lsn, now);

    // Check if chunk should be sealed
    if (chunk.should_seal(m_chunk_size_threshold)) {
        seal_and_rotate_chunk(key);
    }

    // Generate sequential ID for backwards compatibility
    types::AtomId atom_id = generate_sequential_id();

    // Return atom (for backwards compatibility)
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

TemporalChunk& AtomLog::get_or_create_active_chunk(const TemporalKey& key) {
    // Check if active chunk exists
    auto it = m_active_chunks.find(key);
    if (it != m_active_chunks.end()) {
        return it->second;
    }

    // Get next chunk ID for this stream
    types::ChunkId chunk_id = 0;
    auto chunk_id_it = m_next_chunk_id.find(key);
    if (chunk_id_it != m_next_chunk_id.end()) {
        chunk_id = chunk_id_it->second++;
    } else {
        m_next_chunk_id[key] = 1;  // Next will be 1
    }

    // Create new active chunk
    types::LogSequenceNumber current_lsn{m_next_lsn + 1};
    types::Timestamp now = get_current_timestamp();

    auto [new_it, inserted] = m_active_chunks.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(key),
        std::forward_as_tuple(chunk_id, key.entity_id, key.tag, current_lsn, now)
    );

    return new_it->second;
}

void AtomLog::seal_and_rotate_chunk(const TemporalKey& key) {
    // Find active chunk
    auto it = m_active_chunks.find(key);
    if (it == m_active_chunks.end()) {
        return;  // No active chunk to seal
    }

    TemporalChunk& chunk = it->second;

    // Seal the chunk
    types::LogSequenceNumber final_lsn{m_next_lsn};
    types::Timestamp now = get_current_timestamp();
    chunk.seal(final_lsn, now);

    // Move to sealed chunks
    types::ChunkId chunk_id = chunk.metadata().chunk_id;
    m_sealed_chunks.emplace(chunk_id, std::move(chunk));

    // Remove from active chunks
    m_active_chunks.erase(it);

    // Note: Next call to get_or_create_active_chunk() will create a new chunk
}

} // namespace gtaf::core
