#include "temporal_chunk.h"
#include <stdexcept>

namespace gtaf::core {

// ---- TemporalChunk Implementation ----

TemporalChunk::TemporalChunk(
    types::ChunkId chunk_id,
    types::EntityId entity_id,
    std::string tag,
    types::LogSequenceNumber start_lsn,
    types::Timestamp created_at
)
    : m_metadata{
        chunk_id,
        entity_id,
        std::move(tag),
        start_lsn,
        start_lsn,  // end_lsn starts same as start_lsn
        created_at,
        0,          // sealed_at = 0 (not sealed yet)
        0,          // value_count = 0
        false       // is_sealed = false
    }
{
    // Reserve space to minimize reallocations
    m_values.reserve(1000);
    m_timestamps.reserve(1000);
    m_lsns.reserve(1000);
}

void TemporalChunk::append(
    types::AtomValue value,
    types::LogSequenceNumber lsn,
    types::Timestamp timestamp
) {
    if (m_metadata.is_sealed) {
        throw std::logic_error("Cannot append to sealed chunk");
    }

    m_values.push_back(std::move(value));
    m_timestamps.push_back(timestamp);
    m_lsns.push_back(lsn);

    m_metadata.end_lsn = lsn;  // Update end LSN
    ++m_metadata.value_count;
}

bool TemporalChunk::should_seal(size_t threshold) const noexcept {
    return m_metadata.value_count >= threshold;
}

void TemporalChunk::seal(types::LogSequenceNumber final_lsn, types::Timestamp sealed_at) {
    if (m_metadata.is_sealed) {
        throw std::logic_error("Chunk already sealed");
    }

    m_metadata.end_lsn = final_lsn;
    m_metadata.sealed_at = sealed_at;
    m_metadata.is_sealed = true;

    // Shrink vectors to exact size (no more appends will happen)
    m_values.shrink_to_fit();
    m_timestamps.shrink_to_fit();
    m_lsns.shrink_to_fit();
}

const TemporalChunkMetadata& TemporalChunk::metadata() const noexcept {
    return m_metadata;
}

bool TemporalChunk::is_sealed() const noexcept {
    return m_metadata.is_sealed;
}

size_t TemporalChunk::value_count() const noexcept {
    return m_metadata.value_count;
}

const std::vector<types::AtomValue>& TemporalChunk::values() const noexcept {
    return m_values;
}

const std::vector<types::Timestamp>& TemporalChunk::timestamps() const noexcept {
    return m_timestamps;
}

const std::vector<types::LogSequenceNumber>& TemporalChunk::lsns() const noexcept {
    return m_lsns;
}

} // namespace gtaf::core
