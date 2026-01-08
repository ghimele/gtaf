#pragma once

#include "../types/types.h"
#include <vector>
#include <string>
#include <optional>

namespace gtaf::core {

/**
 * @brief Delta event for mutable atom changes
 *
 * Records a single mutation to a mutable atom's value.
 * Enables historical reconstruction of state.
 */
struct MutableDelta {
    types::LogSequenceNumber lsn;       // When delta was applied
    types::Timestamp timestamp;         // Wall-clock time
    types::AtomValue old_value;         // Previous value (for rollback)
    types::AtomValue new_value;         // New value after mutation
};

/**
 * @brief Metadata for mutable atom state tracking
 *
 * Tracks identity, snapshot policy, and delta history
 * for a single mutable atom instance.
 */
struct MutableStateMetadata {
    types::AtomId atom_id;                      // Stable identifier
    types::EntityId entity_id;                  // Which entity owns this
    std::string tag;                            // Property name
    types::LogSequenceNumber created_lsn;       // When atom was created
    types::LogSequenceNumber last_snapshot_lsn;  // Last snapshot LSN (default initialized to 0)
    types::Timestamp last_snapshot_time = 0;    // When last snapshot occurred
    uint32_t delta_count_since_snapshot = 0;    // Deltas since last snapshot
};

/**
 * @brief Container for mutable atom state with delta logging
 *
 * Mutable atoms provide:
 * - In-place mutation for performance (counters, aggregates)
 * - Delta logging for history reconstruction
 * - Periodic snapshots for recovery points
 * - Logical immutability despite physical mutation
 *
 * Aligns with ATOM_TAXONOMY.md §6 and WRITE_READ_PIPELINES.md §8
 */
class MutableState {
public:
    /**
     * @brief Construct a new mutable atom state
     *
     * @param atom_id Stable identifier for this mutable atom
     * @param entity_id Entity that owns this atom
     * @param tag Property name (e.g., "login_count")
     * @param initial_value Starting value
     * @param created_lsn Log sequence number at creation
     * @param created_at Creation timestamp
     */
    MutableState(
        types::AtomId atom_id,
        types::EntityId entity_id,
        std::string tag,
        types::AtomValue initial_value,
        types::LogSequenceNumber created_lsn,
        types::Timestamp created_at
    );

    /**
     * @brief Apply a mutation and log the delta
     *
     * @param new_value The new value to set
     * @param lsn Log sequence number for this mutation
     * @param timestamp When this mutation occurred
     */
    void mutate(
        types::AtomValue new_value,
        types::LogSequenceNumber lsn,
        types::Timestamp timestamp
    );

    /**
     * @brief Check if snapshot should be emitted
     *
     * @param delta_threshold Number of deltas before snapshot
     * @return true if delta count >= threshold
     */
    [[nodiscard]] bool should_snapshot(uint32_t delta_threshold) const noexcept;

    /**
     * @brief Mark that a snapshot was emitted
     *
     * Resets delta counter and updates snapshot metadata.
     *
     * @param lsn Log sequence number of snapshot
     * @param timestamp When snapshot was emitted
     */
    void mark_snapshot(
        types::LogSequenceNumber lsn,
        types::Timestamp timestamp
    );

    /**
     * @brief Get current value
     */
    [[nodiscard]] const types::AtomValue& current_value() const noexcept;

    /**
     * @brief Get metadata
     */
    [[nodiscard]] const MutableStateMetadata& metadata() const noexcept;

    /**
     * @brief Get delta history
     *
     * All deltas since last snapshot (or creation if no snapshots yet).
     */
    [[nodiscard]] const std::vector<MutableDelta>& deltas() const noexcept;

    /**
     * @brief Get number of deltas since last snapshot
     */
    [[nodiscard]] uint32_t delta_count() const noexcept;

private:
    MutableStateMetadata m_metadata;
    types::AtomValue m_current_value;           // Current mutable state
    std::vector<MutableDelta> m_deltas;         // Delta history since last snapshot
};

} // namespace gtaf::core
