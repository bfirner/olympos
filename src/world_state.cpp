/*
 * Copyright 2022 Bernhard Firner
 *
 * State of an area in the game.
 */

#include <algorithm>
#include <vector>

#include "world_state.hpp"

using std::vector;

// TODO This may not be the way, but it will work out for now.
void updatePassable(const std::list<Entity>& entities, vector<vector<bool>>& passable) {
    // Set everything passable and then update things that are not.
    for (auto& row : passable) {
        row.assign(true, row.size());
    }
    for (auto& entity : entities) {
        if (entity.traits.contains("impassable")) {
            passable[entity.y][entity.x] = false;
        }
        else if (entity.traits.contains("mob") and
                not entity.traits.contains("small") and
                not entity.traits.contains("flying")) {
            passable[entity.y][entity.x] = false;
        }
    }
}

WorldState::WorldState(size_t field_height, size_t field_width) : passable{field_height, vector<bool>(field_width, true)} {
    this->field_height = field_height;
    this->field_width = field_width;
}

void WorldState::addEntity(size_t y, size_t x, const std::string& name, const std::set<std::string>& traits) {
    entities.push_back({y, x, name, traits});
}

void WorldState::initialize() {
    // Get the player entity
    decltype(entities)::iterator player = std::find_if(entities.begin(), entities.end(),
            [](auto& ent) {return (ent.traits.contains("player"));});
    if (player != entities.end()) {
        named_entities["player"] = player;
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

    updatePassable(entities, passable);
}

void WorldState::update() {
    updatePassable(entities, passable);
}
