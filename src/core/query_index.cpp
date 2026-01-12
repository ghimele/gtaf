#include "query_index.h"
#include <algorithm>
#include <cctype>

namespace gtaf::core {

QueryIndex::QueryIndex(const ProjectionEngine& projector)
    : m_projector(projector) {}

size_t QueryIndex::build_index(const std::string& tag) {
    // Get all entities
    auto entities = m_projector.get_all_entities();

    // Create index for this tag
    auto& index = m_string_indexes[tag];
    index.clear();
    index.reserve(entities.size());

    size_t indexed_count = 0;

    // Iterate through entities and extract the property value
    for (const auto& entity : entities) {
        Node node = m_projector.rebuild(entity);
        auto value = node.get(tag);

        if (value && std::holds_alternative<std::string>(*value)) {
            index[entity] = std::get<std::string>(*value);
            indexed_count++;
        }
    }

    return indexed_count;
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
