#include "mutable_state.h"
#include <stdexcept>

namespace gtaf::core {

// ---- MutableState Implementation ----

MutableState::MutableState(
    types::AtomId atom_id,
    types::EntityId entity_id,
    std::string tag,
    types::AtomValue initial_value,
    types::LogSequenceNumber created_lsn,
    types::Timestamp created_at
)
    : m_metadata{
        atom_id,
        entity_id,
        std::move(tag),
        created_lsn,
        created_lsn,  // last_snapshot_lsn = created_lsn
        created_at,   // last_snapshot_time = created_at
        0             // delta_count_since_snapshot = 0
    },
    m_current_value(std::move(initial_value))
{
    // Reserve space for typical delta history
    m_deltas.reserve(100);
}

void MutableState::mutate(
    types::AtomValue new_value,
    types::LogSequenceNumber lsn,
    types::Timestamp timestamp
) {
    // Record delta (old -> new)
    MutableDelta delta{
        lsn,
        timestamp,
        m_current_value,  // Copy current value as old_value
        new_value         // Copy new value
    };

    // Apply mutation in-place
    m_current_value = std::move(new_value);

    // Append delta to history
    m_deltas.push_back(std::move(delta));

    // Increment delta counter
    ++m_metadata.delta_count_since_snapshot;
}

bool MutableState::should_snapshot(uint32_t delta_threshold) const noexcept {
    return m_metadata.delta_count_since_snapshot >= delta_threshold;
}

void MutableState::mark_snapshot(
    types::LogSequenceNumber lsn,
    types::Timestamp timestamp
) {
    // Update snapshot metadata
    m_metadata.last_snapshot_lsn = lsn;
    m_metadata.last_snapshot_time = timestamp;
    m_metadata.delta_count_since_snapshot = 0;

    // Clear delta history (compaction)
    // After snapshot, we don't need old deltas for recovery
    m_deltas.clear();
}

const types::AtomValue& MutableState::current_value() const noexcept {
    return m_current_value;
}

const MutableStateMetadata& MutableState::metadata() const noexcept {
    return m_metadata;
}

const std::vector<MutableDelta>& MutableState::deltas() const noexcept {
    return m_deltas;
}

uint32_t MutableState::delta_count() const noexcept {
    return m_metadata.delta_count_since_snapshot;
}

} // namespace gtaf::core
