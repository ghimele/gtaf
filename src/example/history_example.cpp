#include "../core/atom_log.h"
#include "../core/projection_engine.h"
#include "../types/hash_utils.h"
#include <iostream>
#include <algorithm>

using namespace gtaf;

int main() {
    std::cout << "=== GTAF History Management Demo ===\n\n";

    core::AtomLog log;

    // Create a user entity
    types::EntityId user{};
    std::fill(user.bytes.begin(), user.bytes.end(), 0);
    user.bytes[0] = 1;

    std::cout << "--- Step 1: Initial Status ---\n";
    auto atom1 = log.append(user, "user.status", std::string("inactive"), types::AtomType::Canonical);
    std::cout << "Created atom: user.status = 'inactive'\n";
    std::cout << "  AtomId: " << types::atom_id_to_hex(atom1.atom_id()) << "\n";
    std::cout << "  Timestamp: " << atom1.created_at() << "\n\n";

    std::cout << "--- Step 2: Update Status ---\n";
    auto atom2 = log.append(user, "user.status", std::string("active"), types::AtomType::Canonical);
    std::cout << "Created atom: user.status = 'active'\n";
    std::cout << "  AtomId: " << types::atom_id_to_hex(atom2.atom_id()) << "\n";
    std::cout << "  Timestamp: " << atom2.created_at() << "\n\n";

    std::cout << "--- Step 3: Another Update ---\n";
    auto atom3 = log.append(user, "user.status", std::string("suspended"), types::AtomType::Canonical);
    std::cout << "Created atom: user.status = 'suspended'\n";
    std::cout << "  AtomId: " << types::atom_id_to_hex(atom3.atom_id()) << "\n";
    std::cout << "  Timestamp: " << atom3.created_at() << "\n\n";

    std::cout << "--- Analysis ---\n";
    std::cout << "Total atoms in log: " << log.all().size() << "\n";
    std::cout << "All atoms have different AtomIds: ";
    if (atom1.atom_id() != atom2.atom_id() && atom2.atom_id() != atom3.atom_id()) {
        std::cout << "YES ✓\n";
    } else {
        std::cout << "NO\n";
    }

    // Get entity references to check LSN ordering
    auto user_refs = log.get_entity_atoms(user);
    std::cout << "Entity has " << user_refs.size() << " atom references\n";
    std::cout << "LSNs are strictly increasing: ";
    if (user_refs.size() >= 3 &&
        user_refs[0].lsn.value < user_refs[1].lsn.value &&
        user_refs[1].lsn.value < user_refs[2].lsn.value) {
        std::cout << "YES ✓\n\n";
    } else {
        std::cout << "NO\n\n";
    }

    std::cout << "--- Projection View (Current State) ---\n";
    core::ProjectionEngine projector(log);
    auto node = projector.rebuild(user);

    auto current_status = node.get("user.status");
    if (current_status.has_value()) {
        std::cout << "Current user.status: '" << std::get<std::string>(*current_status) << "'\n";
        std::cout << "  (This is the value with highest LSN)\n\n";
    }

    std::cout << "--- Full History ---\n";
    std::cout << "The log preserves ALL versions:\n";
    std::cout << "Using entity reference index to retrieve history:\n";
    int version = 1;
    for (const auto& ref : user_refs) {
        const core::Atom* atom = log.get_atom(ref.atom_id);
        if (atom && atom->type_tag() == "user.status") {
            std::cout << "  Version " << version++ << ": '"
                      << std::get<std::string>(atom->value())
                      << "' (LSN: " << ref.lsn.value
                      << ", Timestamp: " << atom->created_at() << ")\n";
        }
    }

    std::cout << "\n--- Key Insights ---\n";
    std::cout << "1. Each update creates a NEW atom (immutable content)\n";
    std::cout << "2. Content is stored once (deduplicated), but entities track references\n";
    std::cout << "3. Entity reference layer tracks which atoms each entity uses\n";
    std::cout << "4. Projection shows LATEST based on per-entity LSN ordering\n";
    std::cout << "5. Time-travel queries are possible by filtering LSN/timestamp\n";
    std::cout << "6. Each distinct value gets a unique content-addressed ID\n\n";

    std::cout << "--- Node History Tracking ---\n";
    const auto& history = node.history();
    std::cout << "Node tracks " << history.size() << " historical atoms for this entity:\n";
    for (size_t i = 0; i < history.size(); ++i) {
        std::cout << "  " << (i + 1) << ". AtomId: "
                  << types::atom_id_to_hex(history[i].first)
                  << " (LSN: " << history[i].second.value << ")\n";
    }

    std::cout << "\n--- Reusing Values (Deduplication) ---\n";
    std::cout << "What if we set status back to 'active'?\n";
    size_t atoms_before = log.all().size();
    auto atom4 = log.append(user, "user.status", std::string("active"), types::AtomType::Canonical);
    size_t atoms_after = log.all().size();

    std::cout << "AtomId of new 'active': " << types::atom_id_to_hex(atom4.atom_id()) << "\n";
    std::cout << "AtomId of old 'active': " << types::atom_id_to_hex(atom2.atom_id()) << "\n";

    if (atom4.atom_id() == atom2.atom_id()) {
        std::cout << "✓ SAME AtomId! Content-addressed deduplication in action.\n";
        std::cout << "  The content is stored once, but the entity reference is tracked separately.\n";
    }

    std::cout << "\nContent atoms before: " << atoms_before << "\n";
    std::cout << "Content atoms after: " << atoms_after << "\n";
    std::cout << "  (No new content atom created - 'active' value already exists)\n";

    auto updated_refs = log.get_entity_atoms(user);
    std::cout << "Entity references: " << updated_refs.size() << "\n";
    std::cout << "  (New reference added for this entity's latest update)\n";

    std::cout << "\n=== Demo Complete ===\n";
    return 0;
}
