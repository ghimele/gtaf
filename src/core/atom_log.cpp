#include "atom_log.h"
#include "../types/hash_utils.h"
#include "persistence.h"
#include <chrono>
#include <algorithm>
#include <iostream>

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

const std::vector<AtomReference>* AtomLog::get_entity_atoms(types::EntityId entity) const {
    auto it = m_entity_refs.find(entity);
    if (it == m_entity_refs.end()) {
        return nullptr;  // No atoms for this entity
    }
    return &it->second;  // Return pointer to avoid copy
}

const Atom* AtomLog::get_atom(types::AtomId atom_id) const {
    auto it = m_content_index.find(atom_id);
    if (it == m_content_index.end()) {
        return nullptr;  // Atom not found
    }
    return &m_atoms[it->second];
}

std::vector<types::EntityId> AtomLog::get_all_entities() const {
    std::vector<types::EntityId> result;
    result.reserve(m_entity_refs.size());
    for (const auto& [entity, refs] : m_entity_refs) {
        result.push_back(entity);
    }
    return result;
}

AtomLog::Stats AtomLog::get_stats() const {
    Stats stats;
    stats.total_atoms = m_atoms.size();
    stats.canonical_atoms = m_canonical_atom_count;
    stats.deduplicated_hits = m_dedup_hits;
    stats.unique_canonical_atoms = m_canonical_dedup_map.size();
    stats.total_entities = m_entity_refs.size();

    // Count total references across all entities
    size_t total_refs = 0;
    for (const auto& [entity, refs] : m_entity_refs) {
        total_refs += refs.size();
    }
    stats.total_references = total_refs;

    return stats;
}

size_t AtomLog::append_batch(const std::vector<BatchAtom>& atoms) {
    if (atoms.empty()) return 0;

    // Get timestamp once for the entire batch
    types::Timestamp batch_timestamp = get_current_timestamp();

    // Phase 1: Pre-calculate how many atoms each entity will receive
    // Use a local map to batch entity references before committing
    std::unordered_map<types::EntityId, std::vector<AtomReference>, EntityIdHash> batch_entity_refs;
    batch_entity_refs.reserve(atoms.size() / 8);  // Estimate unique entities

    // Phase 2: Pre-reserve main storage
    m_atoms.reserve(m_atoms.size() + atoms.size() / 2);

    // Pre-reserve hash maps to avoid rehashing during batch
    size_t estimated_new_atoms = atoms.size() / 2;
    if (m_canonical_dedup_map.bucket_count() < m_canonical_dedup_map.size() + estimated_new_atoms) {
        m_canonical_dedup_map.reserve(m_canonical_dedup_map.size() + estimated_new_atoms);
        m_content_index.reserve(m_content_index.size() + estimated_new_atoms);
        m_refcounts.reserve(m_refcounts.size() + estimated_new_atoms);
    }

    size_t stored_count = 0;

    // Phase 3: Process atoms with minimal map operations
    for (const auto& batch_atom : atoms) {
        // Only support Canonical atoms in batch mode for now
        if (batch_atom.classification != types::AtomType::Canonical) {
            append(batch_atom.entity, batch_atom.tag, batch_atom.value, batch_atom.classification);
            ++stored_count;
            continue;
        }

        // Compute content-based hash
        types::AtomId atom_id = types::compute_content_hash(batch_atom.tag, batch_atom.value);

        // Use insert to do lookup + insert in ONE operation (critical optimization)
        auto [it, inserted] = m_canonical_dedup_map.try_emplace(atom_id, m_atoms.size());

        // Generate LSN
        types::LogSequenceNumber lsn{++m_next_lsn};

        // Batch entity references locally (much faster than direct map access)
        batch_entity_refs[batch_atom.entity].push_back({atom_id, lsn});

        if (inserted) {
            // New atom - store it
            m_atoms.emplace_back(
                atom_id,
                types::AtomType::Canonical,
                batch_atom.tag,
                batch_atom.value,
                batch_timestamp
            );
            m_content_index.emplace(atom_id, it->second);
            m_refcounts.emplace(atom_id, 1);
            ++m_canonical_atom_count;
            ++stored_count;
        } else {
            // Duplicate - just increment counters
            ++m_dedup_hits;
            ++m_refcounts[atom_id];
        }
    }

    // Phase 4: Merge batch entity references into main map (bulk operation)
    for (auto& [entity, refs] : batch_entity_refs) {
        auto& main_refs = m_entity_refs[entity];
        main_refs.reserve(main_refs.size() + refs.size());
        main_refs.insert(main_refs.end(),
                        std::make_move_iterator(refs.begin()),
                        std::make_move_iterator(refs.end()));
    }

    return stored_count;
}

void AtomLog::reserve(size_t atom_count, size_t entity_count) {
    m_atoms.reserve(atom_count);

    // Pre-reserve all hash maps to avoid rehashing during bulk import
    m_canonical_dedup_map.reserve(atom_count);
    m_content_index.reserve(atom_count);
    m_refcounts.reserve(atom_count);

    if (entity_count > 0) {
        m_entity_refs.reserve(entity_count);
    }
}

Atom AtomLog::append_canonical(
    types::EntityId entity,
    std::string tag,
    types::AtomValue value
) {
    // Compute content-based hash
    types::AtomId atom_id = types::compute_content_hash(tag, value);

    // Check for existing atom with same hash (deduplication)
    bool is_new_atom = false;
    if (auto it = m_canonical_dedup_map.find(atom_id); it != m_canonical_dedup_map.end()) {
        // Content already exists - increment refcount and add entity reference
        ++m_dedup_hits;
        ++m_refcounts[atom_id];
    } else {
        // New content - will create atom below
        is_new_atom = true;
    }

    // Add entity reference with per-entity LSN
    types::LogSequenceNumber lsn{++m_next_lsn};
    m_entity_refs[entity].push_back({atom_id, lsn});

    // If new content, create and store atom
    if (is_new_atom) {
        types::Timestamp now = get_current_timestamp();

        Atom atom(
            atom_id,
            types::AtomType::Canonical,
            std::move(tag),
            std::move(value),
            now
        );

        // Store in log, content index, and dedup map
        size_t index = m_atoms.size();
        m_atoms.push_back(atom);
        m_content_index[atom_id] = index;
        m_canonical_dedup_map[atom_id] = index;
        m_refcounts[atom_id] = 1;
        ++m_canonical_atom_count;

        return atom;
    } else {
        // Return existing atom
        return m_atoms[m_canonical_dedup_map[atom_id]];
    }
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

    // Generate sequential ID
    types::AtomId atom_id = generate_sequential_id();

    // Add entity reference with per-entity LSN
    m_entity_refs[entity].push_back({atom_id, lsn});

    // Create atom (content only, no entity_id or lsn in Atom itself)
    Atom atom(
        atom_id,
        types::AtomType::Temporal,
        std::move(tag),
        std::move(value),
        now
    );

    // Store in content index and atoms
    size_t index = m_atoms.size();
    m_atoms.push_back(atom);
    m_content_index[atom_id] = index;

    return atom;
}

Atom AtomLog::append_mutable(
    types::EntityId entity,
    std::string tag,
    types::AtomValue value
) {
    types::LogSequenceNumber lsn{++m_next_lsn};
    types::Timestamp now = get_current_timestamp();

    // Get or create mutable state for this property
    MutableState& state = get_or_create_mutable_state(entity, tag, value);

    // Apply mutation and log delta
    state.mutate(value, lsn, now);

    // Check if snapshot should be emitted
    if (state.should_snapshot(m_snapshot_delta_threshold)) {
        emit_snapshot(state);
    }

    types::AtomId atom_id = state.metadata().atom_id;

    // Add entity reference with per-entity LSN
    m_entity_refs[entity].push_back({atom_id, lsn});

    // Return atom reflecting current state
    Atom atom(
        atom_id,
        types::AtomType::Mutable,
        std::move(tag),
        std::move(value),
        now
    );

    // Store in content index and atoms
    size_t index = m_atoms.size();
    m_atoms.push_back(atom);
    m_content_index[atom_id] = index;

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

MutableState& AtomLog::get_or_create_mutable_state(
    const types::EntityId& entity,
    const std::string& tag,
    const types::AtomValue& initial_value
) {
    // Create key for this mutable property
    TemporalKey key{entity, tag};

    // Check if mutable state already exists
    auto it = m_mutable_states.find(key);
    if (it != m_mutable_states.end()) {
        return it->second;
    }

    // Create new mutable state
    types::AtomId atom_id = generate_sequential_id();
    types::LogSequenceNumber lsn{m_next_lsn};
    types::Timestamp now = get_current_timestamp();

    auto [new_it, inserted] = m_mutable_states.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(key),
        std::forward_as_tuple(atom_id, entity, tag, initial_value, lsn, now)
    );

    return new_it->second;
}

void AtomLog::emit_snapshot(const MutableState& state) {
    // Emit snapshot as a Canonical atom (immutable recovery point)
    const auto& metadata = state.metadata();

    types::LogSequenceNumber lsn{++m_next_lsn};
    types::Timestamp now = get_current_timestamp();

    // Create snapshot atom with special tag marking it as snapshot
    std::string snapshot_tag = metadata.tag + ".snapshot";

    // Snapshots can be content-addressed (deduplicated)
    types::AtomId snapshot_id = types::compute_content_hash(snapshot_tag, state.current_value());

    // Add entity reference for snapshot
    m_entity_refs[metadata.entity_id].push_back({snapshot_id, lsn});

    Atom snapshot_atom(
        snapshot_id,
        types::AtomType::Canonical,  // Snapshots are immutable
        std::move(snapshot_tag),
        state.current_value(),
        now
    );

    // Store snapshot atom
    size_t index = m_atoms.size();
    m_atoms.push_back(snapshot_atom);
    m_content_index[snapshot_id] = index;

    // Mark snapshot in mutable state (clears delta history)
    const_cast<MutableState&>(state).mark_snapshot(lsn, now);

    ++m_snapshot_count;
}

AtomLog::TemporalQueryResult AtomLog::query_temporal_range(
    types::EntityId entity,
    const std::string& tag,
    types::Timestamp start_time,
    types::Timestamp end_time
) const {
    TemporalQueryResult result;
    TemporalKey key{entity, tag};

    // Query sealed chunks
    for (const auto& [chunk_id, chunk] : m_sealed_chunks) {
        const auto& metadata = chunk.metadata();

        // Check if chunk belongs to this stream
        if (metadata.entity_id == entity && metadata.tag == tag) {
            collect_chunk_values(chunk, start_time, end_time, result);
        }
    }

    // Query active chunk (if exists)
    if (auto it = m_active_chunks.find(key); it != m_active_chunks.end()) {
        collect_chunk_values(it->second, start_time, end_time, result);
    }

    result.total_count = result.values.size();
    return result;
}

AtomLog::TemporalQueryResult AtomLog::query_temporal_all(
    types::EntityId entity,
    const std::string& tag
) const {
    // Query with full timestamp range
    return query_temporal_range(entity, tag, 0, UINT64_MAX);
}

void AtomLog::collect_chunk_values(
    const TemporalChunk& chunk,
    types::Timestamp start_time,
    types::Timestamp end_time,
    TemporalQueryResult& result
) const {
    const auto& timestamps = chunk.timestamps();
    const auto& values = chunk.values();
    const auto& lsns = chunk.lsns();

    // Iterate through all values in chunk
    for (size_t i = 0; i < timestamps.size(); ++i) {
        types::Timestamp ts = timestamps[i];

        // Check if timestamp is within range
        if (ts >= start_time && ts <= end_time) {
            result.values.push_back(values[i]);
            result.timestamps.push_back(ts);
            result.lsns.push_back(lsns[i]);
        }
    }
}

bool AtomLog::save(const std::string& filepath) const {
    try {
        BinaryWriter writer(filepath);

        // Write header
        writer.write_bytes("GTAF", 4);  // Magic
        writer.write_u32(2);             // Version 2 (new format with reference layer)
        writer.write_u64(m_next_lsn);
        writer.write_u64(m_next_atom_id);
        writer.write_u64(m_atoms.size());

        // Write all atoms (content only, no entity_id or lsn)
        for (const auto& atom : m_atoms) {
            writer.write_atom_id(atom.atom_id());
            writer.write_u8(static_cast<uint8_t>(atom.classification()));
            writer.write_string(atom.type_tag());
            writer.write_atom_value(atom.value());
            writer.write_timestamp(atom.created_at());
        }

        // Write entity reference layer
        writer.write_u64(m_entity_refs.size());
        for (const auto& [entity, refs] : m_entity_refs) {
            writer.write_entity_id(entity);
            writer.write_u64(refs.size());
            for (const auto& ref : refs) {
                writer.write_atom_id(ref.atom_id);
                writer.write_lsn(ref.lsn);
            }
        }

        // Write refcounts for garbage collection
        writer.write_u64(m_refcounts.size());
        for (const auto& [atom_id, count] : m_refcounts) {
            writer.write_atom_id(atom_id);
            writer.write_u32(count);
        }

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to save: " << e.what() << "\n";
        return false;
    }
}

bool AtomLog::load(const std::string& filepath) {
    try {
        auto t_start = std::chrono::high_resolution_clock::now();
        BinaryReader reader(filepath);

        // Read and verify header
        char magic[4];
        reader.read_bytes(magic, 4);
        if (std::memcmp(magic, "GTAF", 4) != 0) {
            std::cerr << "Invalid file format (bad magic)\n";
            return false;
        }

        uint32_t version = reader.read_u32();
        if (version != 2) {
            std::cerr << "Unsupported version: " << version << " (expected 2)\n";
            return false;
        }

        // Clear current state
        m_atoms.clear();
        m_content_index.clear();
        m_canonical_dedup_map.clear();
        m_entity_refs.clear();
        m_refcounts.clear();
        m_active_chunks.clear();
        m_sealed_chunks.clear();
        m_mutable_states.clear();
        m_next_chunk_id.clear();

        // Read counters
        m_next_lsn = reader.read_u64();
        m_next_atom_id = reader.read_u64();
        uint64_t atom_count = reader.read_u64();

        std::cerr << "[DEBUG] Loading " << atom_count << " atoms...\n";

        // Pre-reserve all hash maps to avoid rehashing during load
        m_atoms.reserve(atom_count);
        m_content_index.reserve(atom_count);
        m_canonical_dedup_map.reserve(atom_count);

        // Track canonical atoms during load (avoids rebuild_indexes scan)
        m_canonical_atom_count = 0;

        auto t_atoms_start = std::chrono::high_resolution_clock::now();

        // Read atoms (content only)
        for (uint64_t i = 0; i < atom_count; ++i) {
            types::AtomId atom_id = reader.read_atom_id();
            types::AtomType type = static_cast<types::AtomType>(reader.read_u8());
            std::string tag = reader.read_string();
            types::AtomValue value = reader.read_atom_value();
            types::Timestamp timestamp = reader.read_timestamp();

            // Reconstruct atom
            m_atoms.emplace_back(atom_id, type, std::move(tag), std::move(value), timestamp);

            // Build indexes inline during load (faster than separate rebuild pass)
            m_content_index.emplace(atom_id, i);
            if (type == types::AtomType::Canonical) {
                m_canonical_dedup_map.emplace(atom_id, i);
                ++m_canonical_atom_count;
            }

            // Progress every 500k atoms
            if ((i + 1) % 500000 == 0) {
                auto now = std::chrono::high_resolution_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - t_atoms_start).count();
                std::cerr << "[DEBUG]   " << (i + 1) << " atoms loaded in " << elapsed << "ms\n";
            }
        }

        auto t_atoms_end = std::chrono::high_resolution_clock::now();
        auto atoms_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t_atoms_end - t_atoms_start).count();
        std::cerr << "[DEBUG] Atoms loaded in " << atoms_ms << "ms\n";

        // Read entity reference layer
        uint64_t entity_count = reader.read_u64();
        std::cerr << "[DEBUG] Loading " << entity_count << " entities...\n";
        m_entity_refs.reserve(entity_count);

        auto t_refs_start = std::chrono::high_resolution_clock::now();

        size_t total_refs_loaded = 0;

        // Temporary buffer for bulk reading references
        // Each ref is 24 bytes: AtomId (16) + LSN (8)
        std::vector<uint8_t> ref_buffer;

        for (uint64_t i = 0; i < entity_count; ++i) {
            types::EntityId entity = reader.read_entity_id();
            uint64_t ref_count = reader.read_u64();

            // Create vector directly in map
            auto& refs = m_entity_refs[entity];
            refs.resize(ref_count);

            if (ref_count > 0) {
                // Bulk read all reference data for this entity
                size_t total_bytes = ref_count * 24;  // 16 + 8 bytes per ref
                ref_buffer.resize(total_bytes);
                reader.read_bytes(ref_buffer.data(), total_bytes);

                // Parse the buffer into AtomReference structs
                const uint8_t* ptr = ref_buffer.data();
                for (uint64_t j = 0; j < ref_count; ++j) {
                    std::memcpy(refs[j].atom_id.bytes.data(), ptr, 16);
                    ptr += 16;
                    std::memcpy(&refs[j].lsn.value, ptr, 8);
                    ptr += 8;
                }
            }
            total_refs_loaded += ref_count;

            // Progress: first at 100, 1000, 10000, then every 100k
            if ((i + 1) == 100 || (i + 1) == 1000 || (i + 1) == 10000 || (i + 1) % 100000 == 0) {
                auto now = std::chrono::high_resolution_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - t_refs_start).count();
                std::cerr << "[DEBUG]   " << (i + 1) << " entities (" << total_refs_loaded << " refs) in " << elapsed << "ms\n";
            }
        }

        auto t_refs_end = std::chrono::high_resolution_clock::now();
        auto refs_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t_refs_end - t_refs_start).count();
        std::cerr << "[DEBUG] Entity refs loaded in " << refs_ms << "ms\n";

        // Read refcounts
        uint64_t refcount_size = reader.read_u64();
        m_refcounts.reserve(refcount_size);
        for (uint64_t i = 0; i < refcount_size; ++i) {
            types::AtomId atom_id = reader.read_atom_id();
            uint32_t count = reader.read_u32();
            m_refcounts.emplace(atom_id, count);
        }

        // Reset session counters (dedup_hits is only meaningful during append)
        m_dedup_hits = 0;
        m_snapshot_count = 0;

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to load: " << e.what() << "\n";
        return false;
    }
}

void AtomLog::rebuild_indexes() {
    // Reset statistics
    m_canonical_atom_count = 0;
    m_dedup_hits = 0;
    m_snapshot_count = 0;

    // Replay the log to rebuild indexes
    for (size_t i = 0; i < m_atoms.size(); ++i) {
        const auto& atom = m_atoms[i];

        // Rebuild canonical dedup map
        if (atom.classification() == types::AtomType::Canonical) {
            // Only index if this is the first occurrence
            if (m_canonical_dedup_map.find(atom.atom_id()) == m_canonical_dedup_map.end()) {
                m_canonical_dedup_map[atom.atom_id()] = i;
                ++m_canonical_atom_count;
            } else {
                ++m_dedup_hits;
            }
        }

        // Note: We don't rebuild temporal chunks or mutable states on load
        // because they're ephemeral/derived structures. If needed, they can
        // be reconstructed by re-appending temporal/mutable atoms, but for
        // simple persistence we only need the canonical dedup map.
    }
}

} // namespace gtaf::core
