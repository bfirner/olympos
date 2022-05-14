/*
 * Copyright 2022 Bernhard Firner
 *
 * State of an area in the game.
 */

#include <algorithm>
#include <cmath>
#include <set>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include "entity.hpp"
#include "lore.hpp"
#include "world_state.hpp"

using std::vector;

bool isPassable(const Entity& entity) {
    // Passable if this is not impassible or a non-small, non-flying mob.
    return not (entity.traits.contains("impassable") or
        (entity.traits.contains("mob") and
         not entity.traits.contains("small") and
         not entity.traits.contains("flying")));

}

// TODO This may not be the way, but it will work out for now.
void updatePassable(const std::list<std::shared_ptr<Entity>>& entities, vector<vector<bool>>& passable) {
    // Set everything passable and then update things that are not.
    for (auto& row : passable) {
        row.assign(true, row.size());
    }
    for (auto& entity_p : entities) {
        if (not isPassable(*entity_p)) {
            passable[entity_p->y][entity_p->x] = false;
        }
    }
}

WorldState::WorldState(size_t field_height, size_t field_width) :
    passable{field_height, vector<bool>(field_width, true)},
    locations{field_height, {field_width, std::list<std::shared_ptr<Entity>>()}} {
    this->field_height = field_height;
    this->field_width = field_width;
}

void WorldState::addEntity(size_t y, size_t x, const std::string& name, const std::set<std::string>& traits) {
    if (y >= this->field_height or x >= this->field_width) {
        throw std::runtime_error("Cannot place entity at "+std::to_string(y)+", "+std::to_string(x)+": out of bounds.");
    }
    // TODO FIXME Make a real constructor for the Entity class
    entities.push_front(std::shared_ptr<Entity>(new Entity{y, x, name, traits, {}}));
    // Fetch the traits from the game lore if some exist for this entity.
    std::set<std::string> base_traits = OlymposLore::getNamedEntry(*entities.front(), "is a");
    entities.front()->traits.insert(base_traits.begin(), base_traits.end());
    // Calculate starting stats for this entity (if it has any)
    std::optional<Stats> stats = OlymposLore::getStats(*entities.front());
    if (stats) {
        entities.front()->stats = stats.value();
    }

    locations[y][x].push_back(entities.front());
}

bool WorldState::moveEntity(Entity& entity, size_t y, size_t x) {
    // Out of bounds? Return false.
    if (y >= this->field_height or x >= this->field_height) {
        return false;
    }
    // Return false if the entity cannot move to the given location.
    if (not passable[y][x]) {
        return false;
    }
    // Otherwise move the entity and update the passable and locations members.
    std::list<std::shared_ptr<Entity>>& location_list = locations.at(entity.y).at(entity.x);
    auto this_entity =
        std::find_if(location_list.begin(), location_list.end(),
                [&entity](auto& other) {return other.get() == &entity; });
    // TODO If this entity was not at the location that it thought it was then this was a big mess.
    if (this_entity == location_list.end()) {
        throw std::runtime_error("Messed up locations, dying.");
    }
    // Should be checking for this_entity being the end of the list though.
    // TODO This entire locations member variable is a mess.
    location_list.remove_if([&entity](auto& other) {
            return other.get() == &entity; });

    // Update passable with this entity removed.
    passable[entity.y][entity.x] = std::all_of(location_list.begin(), location_list.end(), [](auto& ent_iter){return isPassable(*ent_iter);});

    // Now that the old location is updated, move into the new location.
    entity.y = y;
    entity.x = x;
    locations[y][x].push_back(*this_entity);
    // Update passable at the new location.
    {
        std::list<std::shared_ptr<Entity>>& location_list = locations.at(entity.y).at(entity.x);
        passable[entity.y][entity.x] = std::all_of(location_list.begin(), location_list.end(), [](auto& ent_iter){return isPassable(*ent_iter);});
    }
    return true;
}

void WorldState::initialize() {
    // Get the player entity
    decltype(entities)::iterator player = std::find_if(entities.begin(), entities.end(),
            [](auto& ent) {return (ent->traits.contains("player"));});
    if (player != entities.end()) {
        named_entities["player"] = *player;
    }

    // Make the walls
    for (size_t x = 0; x < field_width; ++x) {
        addEntity(0, x, "Wall", {"wall", "impassable"});
        addEntity(field_height-1, x, "Wall", {"wall", "impassable"});
    }
    // Don't repeat the corners that were already filled in
    for (size_t y = 1; y < field_height-1; ++y) {
        addEntity(y, 0, "Wall", {"wall", "impassable"});
        addEntity(y, field_width-1, "Wall", {"wall", "impassable"});
    }

    // Initialize HP and Mana
    for (auto& entity_p : entities) {
        if (entity_p->stats) {
            Stats& stats = entity_p->stats.value();
            stats.health = std::floor(stats.vitality*0.8 + stats.domain*0.2);
            stats.mana = stats.pool_volume;
        }
    }

    updatePassable(entities, passable);
}

void WorldState::update() {
    updatePassable(entities, passable);
}
