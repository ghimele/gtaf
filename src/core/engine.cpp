#include "../include/core/engine.h"
#include <functional>
#include <mutex>
#include <sstream>
#include <iomanip>
#include <iostream>

namespace gtaf::core {

    // --- 1. Node Management ---

    void Engine::register_node(const core::Node& node) 
    {
        std::unique_lock lock(m_mutex);
        m_node_store.insert_or_assign(node.id(), node);
    }

    // 2. Atom Management
    /**
     * @brief Helper to create a Canonical Atom ID from its content.
     * This ensures that if the content is the same, the ID is the same.
     */
    std::string Engine::generate_canonical_atom_id(const std::string& tag, const types::AtomValue& value) {
        // In a real system, you would use a cryptographic hash like SHA-256 here.
        // For this skeleton, we use a placeholder 'can::' prefix.

        std::hash<std::string> string_hasher;
        size_t seed = string_hasher(tag);

        // Visit the variant to mix the value into the hash
        std::visit([&seed](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (!std::is_same_v<T, std::monostate>) {
                if constexpr (requires { std::hash<T>{}(arg); }) {
                    seed ^= std::hash<T>{}(arg) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
                } else {
                    // Handle non-hashable types here (e.g., skip or throw)
                    // For now, skip them
                }
            }
        }, value);

        std::stringstream ss;
        ss << "can::" << std::hex << std::setw(16) << std::setfill('0') << seed;
        return ss.str();
    }

    std::shared_ptr<Atom> Engine::get_or_create_canonical_atom(const std::string& type_tag, const types::AtomValue& value) 
    {
        // 1. Generate the Identity based on Content
        std::string atom_id = generate_canonical_atom_id(type_tag, value);

        // We use the double-checked locking pattern

        // 2. Thread-safe lookup (shared lock)
        {
            std::shared_lock lock(m_mutex);
            auto it = m_atom_store.find(atom_id);
            if (it != m_atom_store.end()) {
                return it->second;
            }
        }

        // 3. Create if missing (unique lock)
        {
            std::unique_lock lock(m_mutex);
            auto it = m_atom_store.find(atom_id);
            if (it != m_atom_store.end()) {
                return it->second;
            }

            auto new_atom = std::make_shared<core::Atom>(
                atom_id, 
                types::AtomType::Canonical, 
                type_tag, 
                value
            );

            m_atom_store[atom_id] = new_atom;
            return new_atom;
        }
    }

    // 3. Projection
    std::unordered_map<std::string, types::AtomValue> Engine::project_node(const types::NodeID& node_id)
    {
        std::unordered_map<std::string, types::AtomValue> projection;

        std::shared_lock lock(m_mutex);

        // 1. Find the Node
        auto node_it = m_node_store.find(node_id);
        if (node_it == m_node_store.end()) 
        {
            return projection; // Return empty if node doesn't exist
        }

        const auto& node = node_it->second;

        // 2. Resolve every Property (Atom ID -> Atom Value)
        for (const auto& [prop_name, atom_id] : node.properties()) 
        {
            auto atom_it = m_atom_store.find(atom_id);
            if (atom_it != m_atom_store.end()) 
            {
                // We project the VALUE of the atom, not the atom object itself
                projection[prop_name] = atom_it->second->value();
                std::cout << "[DEBUG] Projected property '" << prop_name << "' with Atom ID '" << atom_id << "'\n";
                std::cout << "[DEBUG] Projected ValueType Index '" << atom_it->second->value().index() << "' and TypeTag '" << atom_it->second->type_tag() <<"'\n";
            }
        }
        return projection;
    }

} // namespace gtaf::core