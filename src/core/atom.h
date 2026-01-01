#pragma once
#include "../types/types.h"

namespace gtaf::core {

class Atom {
public:
    Atom(std::string id, 
         types::AtomType cls, 
         std::string tag, 
         types::AtomValue val)
        : m_id(std::move(id)), 
          m_classification(cls), 
          m_type_tag(std::move(tag)), 
          m_value(std::move(val)), 
          m_created_at(0) {} // In production, use high-res clock

    // Accessors
    [[nodiscard]] const std::string& id() const { return m_id; }
    [[nodiscard]] types::AtomType classification() const { return m_classification; }
    [[nodiscard]] const std::string& type_tag() const { return m_type_tag; }
    [[nodiscard]] const types::AtomValue& value() const { return m_value; }
    [[nodiscard]] types::Timestamp created_at() const { return m_created_at; }

    // convenience helpers
    bool is_canonical() const noexcept {
        return m_classification == types::AtomType::Canonical;
    }

    bool is_temporal() const noexcept {
        return m_classification == types::AtomType::Temporal;
    }

    bool is_mutable() const noexcept {
        return m_classification == types::AtomType::Mutable;
    }

private:
    std::string m_id;
    types::AtomType m_classification;
    std::string m_type_tag;
    types::AtomValue m_value;
    types::Timestamp m_created_at;
};

} // namespace gtaf::core
