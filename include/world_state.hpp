/*
 * Copyright 2022 Bernhard Firner
 *
 * State of an area in the game.
 */

#pragma once

#include <list>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "entity.hpp"

struct WorldState {
    std::list<std::shared_ptr<Entity>> entities;
    std::map<std::string, std::shared_ptr<Entity>> named_entities;

    size_t field_height;
    size_t field_width;

    // Keep track of what is passable.
    std::vector<std::vector<bool>> passable;

    // Keep track of mob locations to handle complex interactions.
    std::vector<std::vector<std::list<std::shared_ptr<Entity>>>> locations;

    WorldState(size_t field_height, size_t field_width);
    void addEntity(size_t y, size_t x, const std::string& name, const std::set<std::string>& traits);

    // Returns true if the mob is moved, false otherwise.
    bool moveEntity(Entity& entity, size_t y, size_t x);

    // Initialize layers, such as passable areas, and named entities.
    void initialize();

    // Update layers and entities.
    void update();
};
