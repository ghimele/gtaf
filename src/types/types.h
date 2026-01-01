#pragma once

#include <variant>
#include <vector>
#include <string>
#include <cstdint>

namespace gtaf::types {

// --- 1. Atom Classification ---
/**
 * @brief Defines the storage and deduplication behavior of an Atom.
 */
enum class AtomType : std::uint8_t {
    Canonical = 0, // Immutable, content-addressed, globally deduplicated.
    Temporal  = 1, // Append-only, time-series data, chunked storage.
    Mutable   = 2  // Logged deltas, periodically snapshotted for performance.
};

// --- 2. Primitive Type Aliases ---
using Timestamp   = uint64_t;           // Microseconds since epoch
using Hash        = std::string;        // Content-addressed ID (usually SHA-256)
using NodeID      = std::string;        // Unique Identity anchor
using Vector      = std::vector<float>; // AI/ML Embedding (e.g., 768 dims)

// --- 3. The Atom Value Variant ---
/**
 * @brief The universal container for GTAF data.
 */
using AtomValue = std::variant<
    std::monostate,  // Representing 'Null'
    bool,
    int64_t,
    double,
    std::string,
    Vector,
    std::vector<uint8_t> // Binary Blobs
>;

} // namespace gtaf::types