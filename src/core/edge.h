#pragma once
#include "../types/types.h"

namespace gtaf::core {

class Edge {
public:
    Edge(types::NodeID from, types::NodeID to, std::string rel)
        : m_from(std::move(from)), m_to(std::move(to)), m_relation(std::move(rel)) {}

    [[nodiscard]] const types::NodeID& from() const { return m_from; }
    [[nodiscard]] const types::NodeID& to() const { return m_to; }
    [[nodiscard]] const std::string& relation() const { return m_relation; }

private:
    types::NodeID m_from;
    types::NodeID m_to;
    std::string m_relation;
};

} // namespace gtaf::core