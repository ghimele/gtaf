#include "query_index.h"
#include <algorithm>
#include <cctype>
#include <iostream>

namespace gtaf::core {

QueryIndex::QueryIndex(const ProjectionEngine& projector)
    : m_projector(&projector), m_log(nullptr) {}

QueryIndex::QueryIndex(const AtomLog& log)
    : m_projector(nullptr), m_log(&log) {}

size_t QueryIndex::build_indexes_direct(const std::vector<std::string>& tags) {
    if (!m_log || tags.empty()) {
        return 0;
    }

    const size_t num_tags = tags.size();

    // Create tag -> index mapping for O(1) lookup
    std::unordered_map<std::string, size_t> tag_to_index;
    tag_to_index.reserve(num_tags);
    for (size_t i = 0; i < num_tags; ++i) {
        tag_to_index[tags[i]] = i;
    }

    // Get all entities
    auto entities = m_log->get_all_entities();

    // Pre-create and reserve indexes for all requested tags
    for (const auto& tag : tags) {
        auto& index = m_string_indexes[tag];
        index.clear();
        index.reserve(entities.size());
    }

    // Track latest value per tag using flat arrays (faster than hash map per entity)
    struct LatestValue {
        std::string value;
        uint64_t lsn = 0;
        bool has_value = false;
    };

    // Reuse these vectors across entities to avoid repeated allocations
    std::vector<LatestValue> latest_values(num_tags);

    size_t total_indexed = 0;
    size_t entities_processed = 0;

    // Process each entity directly
    for (const auto& entity : entities) {
        // Reset latest values for this entity
        for (size_t i = 0; i < num_tags; ++i) {
            latest_values[i].has_value = false;
            latest_values[i].lsn = 0;
        }

        // Get atom references for this entity
        const auto* refs = m_log->get_entity_atoms(entity);
        if (!refs) continue;

        // Scan atoms and track latest value per tag
        for (const auto& ref : *refs) {
            const Atom* atom = m_log->get_atom(ref.atom_id);
            if (!atom) continue;

            const std::string& type_tag = atom->type_tag();

            // Only process tags we're interested in
            auto it = tag_to_index.find(type_tag);
            if (it == tag_to_index.end()) continue;

            size_t idx = it->second;

            // Check if this is newer than what we have
            if (!latest_values[idx].has_value || ref.lsn.value > latest_values[idx].lsn) {
                // Extract string value
                const auto& value = atom->value();
                if (std::holds_alternative<std::string>(value)) {
                    latest_values[idx].value = std::get<std::string>(value);
                    latest_values[idx].lsn = ref.lsn.value;
                    latest_values[idx].has_value = true;
                }
            }
        }

        // Store results in indexes
        for (size_t i = 0; i < num_tags; ++i) {
            if (latest_values[i].has_value) {
                m_string_indexes[tags[i]][entity] = std::move(latest_values[i].value);
                total_indexed++;
            }
        }

        entities_processed++;
        if (entities_processed % 100000 == 0) {
            std::cerr << "[DEBUG] Index: processed " << entities_processed << " entities..." << std::endl;
        }
    }

    return total_indexed;
}

size_t QueryIndex::build_index(const std::string& tag) {
    // Use the multi-tag version for consistency
    return build_indexes({tag});
}

size_t QueryIndex::build_indexes(const std::vector<std::string>& tags) {
    if (tags.empty()) {
        return 0;
    }

    // Use direct log access if available (much faster)
    if (m_log) {
        return build_indexes_direct(tags);
    }

    // Fall back to ProjectionEngine approach
    if (!m_projector) {
        return 0;
    }

    // Get all entities
    auto entities = m_projector->get_all_entities();

    // Pre-create and reserve indexes for all requested tags
    for (const auto& tag : tags) {
        auto& index = m_string_indexes[tag];
        index.clear();
        index.reserve(entities.size());
    }

    size_t total_indexed = 0;

    // Use streaming to rebuild nodes once and extract all tag values
    m_projector->rebuild_all_streaming([&](const types::EntityId& entity, const Node& node) {
        for (const auto& tag : tags) {
            auto value = node.get(tag);
            if (value && std::holds_alternative<std::string>(*value)) {
                m_string_indexes[tag][entity] = std::get<std::string>(*value);
                total_indexed++;
            }
        }
    });

    return total_indexed;
}

std::vector<types::EntityId> QueryIndex::find_contains(
    const std::string& tag,
    const std::string& substring
) const {
    std::vector<types::EntityId> results;

    auto it = m_string_indexes.find(tag);
    if (it == m_string_indexes.end()) {
        return results;  // Tag not indexed
    }

    // Convert substring to uppercase for case-insensitive search
    std::string upper_substring = substring;
    std::transform(upper_substring.begin(), upper_substring.end(),
                   upper_substring.begin(), ::toupper);

    const auto& index = it->second;
    results.reserve(index.size() / 10);  // Estimate

    for (const auto& [entity, value] : index) {
        // Convert value to uppercase
        std::string upper_value = value;
        std::transform(upper_value.begin(), upper_value.end(),
                       upper_value.begin(), ::toupper);

        if (upper_value.find(upper_substring) != std::string::npos) {
            results.push_back(entity);
        }
    }

    return results;
}

std::vector<types::EntityId> QueryIndex::find_int_where(
    const std::string& tag,
    std::function<bool(int64_t)> predicate
) const {
    std::vector<types::EntityId> results;

    auto it = m_string_indexes.find(tag);
    if (it == m_string_indexes.end()) {
        return results;  // Tag not indexed
    }

    const auto& index = it->second;
    results.reserve(index.size() / 10);  // Estimate

    for (const auto& [entity, value] : index) {
        if (!value.empty()) {
            try {
                int64_t int_value = std::stoll(value);
                if (predicate(int_value)) {
                    results.push_back(entity);
                }
            } catch (...) {
                // Skip invalid integers
            }
        }
    }

    return results;
}

std::vector<types::EntityId> QueryIndex::find_equals(
    const std::string& tag,
    const std::string& value
) const {
    std::vector<types::EntityId> results;

    auto it = m_string_indexes.find(tag);
    if (it == m_string_indexes.end()) {
        return results;  // Tag not indexed
    }

    const auto& index = it->second;
    results.reserve(index.size() / 10);  // Estimate

    for (const auto& [entity, indexed_value] : index) {
        if (indexed_value == value) {
            results.push_back(entity);
        }
    }

    return results;
}

std::optional<std::string> QueryIndex::get_string(
    const std::string& tag,
    const types::EntityId& entity
) const {
    auto tag_it = m_string_indexes.find(tag);
    if (tag_it == m_string_indexes.end()) {
        return std::nullopt;
    }

    const auto& index = tag_it->second;
    auto entity_it = index.find(entity);
    if (entity_it == index.end()) {
        return std::nullopt;
    }

    return entity_it->second;
}

bool QueryIndex::is_indexed(const std::string& tag) const {
    return m_string_indexes.find(tag) != m_string_indexes.end();
}

QueryIndex::IndexStats QueryIndex::get_stats() const {
    IndexStats stats;
    stats.num_indexed_tags = m_string_indexes.size();
    stats.num_indexed_entities = 0;
    stats.total_entries = 0;

    for (const auto& [tag, index] : m_string_indexes) {
        stats.total_entries += index.size();
        if (index.size() > stats.num_indexed_entities) {
            stats.num_indexed_entities = index.size();
        }
    }

    return stats;
}

} // namespace gtaf::core
