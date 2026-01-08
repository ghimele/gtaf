#pragma once

#include "../types/types.h"
#include <vector>
#include <string>

namespace gtaf::core {

/**
 * @brief Metadata for a temporal chunk
 *
 * Tracks identity, LSN range, and sealing state for a chunk
 * of temporal atom values.
 */
struct TemporalChunkMetadata {
    types::ChunkId chunk_id;                // Sequential within (entity, tag)
    types::EntityId entity_id;              // Which entity owns this chunk
    std::string tag;                        // Which property (e.g., "temperature")
    types::LogSequenceNumber start_lsn;     // First LSN in chunk
    types::LogSequenceNumber end_lsn;       // Last LSN in chunk
    types::Timestamp created_at;            // When chunk started
    types::Timestamp sealed_at = 0;         // When sealed (0 if active)
    uint32_t value_count = 0;               // How many values stored
    bool is_sealed = false;                 // Immutability flag
};

/**
 * @brief Container for temporal atom values with chunk-level semantics
 *
 * Temporal chunks store high-frequency time-series data with:
 * - Sequential appends (no random writes)
 * - Immutability once sealed
 * - No per-value hashing (only chunk-level)
 * - LSN and timestamp tracking for each value
 *
 * Aligns with ATOM_TAXONOMY.md §5 and WRITE_READ_PIPELINES.md §7
 */
class TemporalChunk {
public:
    /**
     * @brief Construct a new active chunk
     *
     * @param chunk_id Sequential identifier for this chunk
     * @param entity_id Entity that owns this chunk
     * @param tag Property name (e.g., "sensor.temperature")
     * @param start_lsn Starting log sequence number
     * @param created_at Creation timestamp
     */
    TemporalChunk(
        types::ChunkId chunk_id,
        types::EntityId entity_id,
        std::string tag,
        types::LogSequenceNumber start_lsn,
        types::Timestamp created_at
    );

    /**
     * @brief Append a value to the active chunk
     *
     * @param value The temporal value to store
     * @param lsn Log sequence number for this value
     * @param timestamp When this value was created
     *
     * @throws std::logic_error if chunk is already sealed
     */
    void append(
        types::AtomValue value,
        types::LogSequenceNumber lsn,
        types::Timestamp timestamp
    );

    /**
     * @brief Check if chunk should be sealed based on threshold
     *
     * @param threshold Maximum values per chunk
     * @return true if value_count >= threshold
     */
    [[nodiscard]] bool should_seal(size_t threshold) const noexcept;

    /**
     * @brief Seal the chunk (make immutable)
     *
     * @param final_lsn Last LSN in this chunk
     * @param sealed_at Seal timestamp
     *
     * @throws std::logic_error if already sealed
     */
    void seal(types::LogSequenceNumber final_lsn, types::Timestamp sealed_at);

    /**
     * @brief Get chunk metadata
     */
    [[nodiscard]] const TemporalChunkMetadata& metadata() const noexcept;

    /**
     * @brief Check if chunk is sealed (immutable)
     */
    [[nodiscard]] bool is_sealed() const noexcept;

    /**
     * @brief Get number of values in chunk
     */
    [[nodiscard]] size_t value_count() const noexcept;

    /**
     * @brief Get all values (for range queries)
     */
    [[nodiscard]] const std::vector<types::AtomValue>& values() const noexcept;

    /**
     * @brief Get all timestamps (for range queries)
     */
    [[nodiscard]] const std::vector<types::Timestamp>& timestamps() const noexcept;

    /**
     * @brief Get all LSNs (for range queries)
     */
    [[nodiscard]] const std::vector<types::LogSequenceNumber>& lsns() const noexcept;

private:
    TemporalChunkMetadata m_metadata;
    std::vector<types::AtomValue> m_values;
    std::vector<types::Timestamp> m_timestamps;
    std::vector<types::LogSequenceNumber> m_lsns;
};

} // namespace gtaf::core
