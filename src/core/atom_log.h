// atom_log.h
#pragma once
#include "atom.h"
#include <vector>
#include <chrono>
#include <cstring>
#include <algorithm>

namespace gtaf::core {

class AtomLog {
public:
    Atom append(
        types::EntityId entity,
        std::string tag,
        types::AtomValue value,
        types::AtomType classification = types::AtomType::Canonical
    ) {
        types::AtomId atom_id;
        std::fill(atom_id.bytes.begin(), atom_id.bytes.end(), 0);
        // Simple incrementing ID for demonstration
        uint64_t id_val = ++m_next_atom_id;
        std::memcpy(atom_id.bytes.data(), &id_val, sizeof(id_val));

        types::LogSequenceNumber lsn{++m_next_lsn};
        types::Timestamp now = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();

        Atom atom(
            atom_id,
            entity,
            classification,
            std::move(tag),
            std::move(value),
            lsn,
            now
        );
        m_atoms.push_back(atom);
        return atom;
    }

    const std::vector<Atom>& all() const { return m_atoms; }

private:
    uint64_t m_next_atom_id = 0;
    uint64_t m_next_lsn = 0;
    std::vector<Atom> m_atoms;
};

} // namespace gtaf::core
