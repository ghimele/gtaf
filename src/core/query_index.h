#pragma once

#include "../types/types.h"
#include "projection_engine.h"
#include "atom_log.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>
#include <functional>

namespace gtaf::core {

/**
 * @brief Query index for fast filtering without full node materialization
 *
 * Indexes store only the indexed field values, not full nodes.
 * This dramatically reduces memory while enabling fast filtering.
 */
class QueryIndex {
public:
    /**
     * @brief Construct a query index from a projection engine
     */
    explicit QueryIndex(const ProjectionEngine& projector);

    /**
     * @brief Construct a query index from an atom log (direct access, faster)
     */
    explicit QueryIndex(const AtomLog& log);

    /**
     * @brief Build an index for a specific property tag
     *
     * @param tag The property tag to index (e.g., "workrequest.description")
     * @return Number of entities indexed
     */
    size_t build_index(const std::string& tag);

    /**
     * @brief Build indexes for multiple property tags in a single pass
     *
     * This is much more efficient than calling build_index() multiple times,
     * as it only rebuilds each node once instead of once per tag.
     *
     * @param tags Vector of property tags to index
     * @return Total number of index entries created
     */
    size_t build_indexes(const std::vector<std::string>& tags);

    /**
     * @brief Get all entity IDs where a string field contains a substring (case-insensitive)
     *
     * @param tag The property tag
     * @param substring The substring to search for
     * @return Vector of matching entity IDs
     */
    std::vector<types::EntityId> find_contains(const std::string& tag, const std::string& substring) const;

    /**
     * @brief Get all entity IDs where an integer field matches a condition
     *
     * @param tag The property tag
     * @param predicate Function that returns true if the value matches
     * @return Vector of matching entity IDs
     */
    std::vector<types::EntityId> find_int_where(
        const std::string& tag,
        std::function<bool(int64_t)> predicate
    ) const;

    /**
     * @brief Get all entity IDs where a string field equals a value
     *
     * @param tag The property tag
     * @param value The exact value to match
     * @return Vector of matching entity IDs
     */
    std::vector<types::EntityId> find_equals(const std::string& tag, const std::string& value) const;

    /**
     * @brief Get indexed string value for an entity
     *
     * @param tag The property tag
     * @param entity The entity ID
     * @return The value if indexed, nullopt otherwise
     */
    std::optional<std::string> get_string(const std::string& tag, const types::EntityId& entity) const;

    /**
     * @brief Check if a tag has been indexed
     */
    bool is_indexed(const std::string& tag) const;

    /**
     * @brief Get statistics about the index
     */
    struct IndexStats {
        size_t num_indexed_tags;
        size_t num_indexed_entities;
        size_t total_entries;
    };
    IndexStats get_stats() const;

private:
    /**
     * @brief Build indexes by directly scanning atom log (bypasses Node reconstruction)
     *
     * Much faster than going through ProjectionEngine because:
     * - No Node object allocation
     * - No string-keyed hash map per entity
     * - No history tracking
     * - Only scans for requested tags
     */
    size_t build_indexes_direct(const std::vector<std::string>& tags);

    const ProjectionEngine* m_projector = nullptr;
    const AtomLog* m_log = nullptr;

    // Index: tag -> (entity_id -> string_value)
    std::unordered_map<std::string, std::unordered_map<types::EntityId, std::string, EntityIdHash>> m_string_indexes;
};

} // namespace gtaf::core
