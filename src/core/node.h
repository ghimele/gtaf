#pragma once
#include "../types/types.h"
#include <unordered_map>
#include <optional>

namespace gtaf::core {

class Node {
public:
    explicit Node(types::NodeID id) 
        : m_node_id(std::move(id)), m_created_at(0) {}

    // Methods to manage properties
    void set_property(const std::string& key, const std::string& atom_id) {
        m_properties[key] = atom_id;
    }

    [[nodiscard]] std::optional<std::string> get_property(const std::string& key) const {
        if (auto it = m_properties.find(key); it != m_properties.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    [[nodiscard]] const types::NodeID& id() const { return m_node_id; }
    [[nodiscard]] const std::unordered_map<std::string, std::string>& properties() const { return m_properties; }

private:
    types::NodeID m_node_id;
    types::Timestamp m_created_at;
    std::unordered_map<std::string, std::string> m_properties;
};

} // namespace gtaf::core
