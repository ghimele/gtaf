#include "../core/atom_log.h"
#include "../core/projection_engine.h"
#include <iostream>
#include <algorithm>

using namespace gtaf;

int main() {
    core::AtomLog log;

    // Create EntityIds
    types::EntityId user{};
    std::fill(user.bytes.begin(), user.bytes.end(), 0);
    user.bytes[0] = 1;

    types::EntityId recipe{};
    std::fill(recipe.bytes.begin(), recipe.bytes.end(), 0);
    recipe.bytes[0] = 2;

    log.append(user, "entity.type", std::string("user"));
    log.append(user, "user.name", std::string("Alice"));

    log.append(recipe, "entity.type", std::string("recipe"));
    log.append(recipe, "recipe.title", std::string("Pasta"));

    // edge: user likes recipe
    types::EdgeValue edge{recipe, "likes"};
    log.append(user, "edge.likes", edge);

    core::ProjectionEngine projector(log);

    auto user_node = projector.rebuild(user);
    auto recipe_node = projector.rebuild(recipe);

    std::cout << "User node rebuilt\n";
    std::cout << "Recipe node rebuilt\n";
}
