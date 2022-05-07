/*
 * Copyright 2022 Bernhard Firner
 *
 * State of an area in the game.
 */

#pragma once

#include "entity.hpp"

#include <list>
#include <map>
#include <string>
#include <vector>

struct WorldState {
    std::list<Entity> entities;
    std::map<std::string, decltype(entities)::iterator> named_entities;

    size_t field_height;
    size_t field_width;

    // Keep track of what is passable.
    std::vector<std::vector<bool>> passable;

    WorldState(size_t field_height, size_t field_width);
    void addEntity(size_t y, size_t x, const std::string& name, const std::set<std::string>& traits);

    // Initialize layers, such as passable areas, and named entities.
    void initialize();

    // Update layers and entities.
    void update();
};
