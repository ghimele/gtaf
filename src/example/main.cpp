#include <iostream>
#include <iomanip>
#include "../core/engine.h"
#include "../core/node.h"
#include "version.h"

using namespace gtaf;

void print_projection(const std::string& title, const std::unordered_map<std::string, types::AtomValue>& data) {
    std::cout << "\n--- " << title << " ---\n";
    for (const auto& [key, val] : data) {
        std::cout << std::left << std::setw(12) << key << " : ";
        // Simple visitor to print the variant
        std::visit([](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, std::monostate>) std::cout << "[null]";
            else if constexpr (std::is_same_v<T, std::string>) std::cout << arg;
            else if constexpr (std::is_same_v<T, int64_t>) std::cout << arg;
            else if constexpr (std::is_same_v<T, double>) std::cout << arg;
            else if constexpr (std::is_same_v<T, bool>) std::cout << (arg ? "true" : "false");
            else if constexpr (std::is_same_v<T, types::Vector>) std::cout << "[Vector Float]";
        }, val);
        std::cout << "\n";
    }
}

int main() {
    core::Engine gtaf_engine;

    std::cout << "GTAF version: " << GTAF_VERSION << std::endl;
    // 1. Create a "Canonical" atom for a common status
    // Both users will share this same physical atom in memory
    auto active_status = gtaf_engine.get_or_create_canonical_atom("user_status", "ACTIVE");
    std::cout << "Canonical Atom ID: " << active_status->id() << "\n";

    // 2. Setup User A
    core::Node user_a("user::001");
    user_a.set_property("name", gtaf_engine.get_or_create_canonical_atom("display_name", "Alice")->id());
    user_a.set_property("status", active_status->id());
    gtaf_engine.register_node(user_a);
    std::cout << "Registered Node ID: " << user_a.id() << "\n";    
    std::cout << "Canonical Atom ID for user_status property: " << user_a.properties().find("status")->second << "\n";


    // 3. Setup User B (Sharing the SAME status atom)
    core::Node user_b("user::002");
    user_b.set_property("name", gtaf_engine.get_or_create_canonical_atom("display_name", "Bob")->id());
    user_b.set_property("status", gtaf_engine.get_or_create_canonical_atom("user_status", "ACTIVE")->id());
    gtaf_engine.register_node(user_b);
    std::cout << "Registered Node ID: " << user_b.id() << "\n";    
    std::cout << "Canonical Atom ID for user_status property: " << user_b.properties().find("status")->second << "\n";

    // 4. Create a "Temporal" atom for a sensor reading (not deduplicated)
    //auto log_entry = gtaf_engine.create_temporal("log_event", "User Alice Logged In");
    //user_a.set_property("last_action", log_entry->id());
   // gtaf_engine.register_node(user_a); // Update node in engine

    // 5. Run the Projection Engine
    auto alice_view = gtaf_engine.project_node("user::001");
    auto bob_view = gtaf_engine.project_node("user::002");
    auto michele_view = gtaf_engine.project_node("user::003");

    print_projection("Alice's Projection", alice_view);
    print_projection("Bob's Projection", bob_view);
    print_projection("Michele's Projection", michele_view);

    // Verification of Deduplication
    if (alice_view["status"] == bob_view["status"]) {
        std::cout << "\n[SUCCESS] Deduplication confirmed: Alice and Bob share the same Status Atom.\n";
    }

    return 0;
}