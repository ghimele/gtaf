#pragma once

#include "./atom.h"
#include "./node.h"
#include "./edge.h"
#include "../types/types.h"

#include <shared_mutex>
#include <unordered_map>
#include <memory>
#include <optional>

namespace gtaf::core {

/**
 * @brief The Registry acts as the Truth Coordinator for GTAF.
 * It manages the lifecycle, deduplication, and thread-safe access to Atoms and Nodes.
 */
class Engine {
public:
    Engine() = default;

    // --- 1. Atom Management ---

    /**
     * @brief Helper to create a Canonical Atom ID from its content.
     * This ensures that if the content is the same, the ID is the same.
     */
    std::string generate_canonical_atom_id(const std::string& tag, const types::AtomValue& value);
    /**
     * @brief Retrieves a Canonical Atom if it exists, otherwise creates it.
     * Enforces the global deduplication contract.
     */
    std::shared_ptr<core::Atom> get_or_create_canonical_atom(
        const std::string& type_tag, 
        const types::AtomValue& value
    );

    /**
     * @brief Creates a Temporal Atom. These are skip-deduplication and are 
     * optimized for high-velocity ingest (IoT/Logs).
     */
    std::shared_ptr<core::Atom> create_temporal(
        const std::string& type_tag, 
        const types::AtomValue& value
    );

    /**
     * @brief Finds an atom by its unique ID.
     */
    [[nodiscard]] std::shared_ptr<core::Atom> get_atom(const std::string& atom_id) const;

    // --- 2. Node & Edge Management ---

    /**
     * @brief Saves or updates a Node in the registry.
     */
    void register_node(const core::Node& node);

    /**
     * @brief Retrieves a Node by its Identity ID.
     */
    [[nodiscard]] std::optional<core::Node> get_node(const types::NodeID& node_id) const;

    /**
     * @brief Links two nodes together.
     */
    void add_edge(const core::Edge& edge);

    // --- 3. Maintenance ---

    /**
     * @brief Background process to reclaim orphaned Canonical atoms.
     */
    size_t perform_gc();

    /**
     * @brief "Hydrates" a node by resolving all its Atom IDs into actual values.
     * Returns a flat map for the application layer.
     */
    [[nodiscard]] std::unordered_map<std::string, types::AtomValue> project_node(const types::NodeID& node_id);

private:
    // Identity storage
    std::unordered_map<types::NodeID, core::Node> m_node_store;
    
    // Value storage (Shared pointers allow deduplication)
    std::unordered_map<std::string, std::shared_ptr<core::Atom>> m_atom_store;
    
    // Index: ContentHash -> AtomID (Used only for Canonical class)
    std::unordered_map<size_t, std::string> m_canonical_index;

    // Thread-safety: Shared mutex allows many readers or one writer
    mutable std::shared_mutex m_mutex;

    // Internal helper for Canonical ID generation
    size_t compute_content_hash(const std::string& tag, const types::AtomValue& val) const;
};

} // namespace gtaf::core