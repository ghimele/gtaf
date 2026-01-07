#pragma once

#include <variant>
#include <vector>
#include <string>
#include <cstdint>
#include <array>
#include <compare>
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

struct AtomId final {
    std::array<uint8_t, 16> bytes;

    // ---- Comparisons ----
    auto operator<=>(const AtomId&) const = default;

    [[nodiscard]] constexpr bool is_nil() const noexcept {
        for (auto b : bytes) {
            if (b != 0) return false;
        }
        return true;
    }
};

struct EntityId final {
    std::array<uint8_t, 16> bytes;

    // ---- Comparisons ----
    auto operator<=>(const EntityId&) const = default;

    [[nodiscard]] constexpr bool is_nil() const noexcept {
        for (auto b : bytes) {
            if (b != 0) return false;
        }
        return true;
    }
};

struct EdgeValue final {
    EntityId target;
    std::string relation;
};

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
    std::vector<uint8_t>, // Binary Blobs
    EdgeValue
>;

struct TransactionId final {
    uint64_t value = 0;

    // ---- Comparisons ----
    auto operator<=>(const TransactionId&) const = default;

    [[nodiscard]] constexpr bool is_auto_commit() const noexcept {
        return value == 0;
    }
};

struct LogSequenceNumber final {
    uint64_t value = 0;

    auto operator<=>(const LogSequenceNumber&) const = default;

    [[nodiscard]] constexpr bool is_valid() const noexcept {
        return value != 0;
    }
};

} // namespace gtaf::types