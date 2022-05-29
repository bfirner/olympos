/*
 * Copyright 2022 Bernhard Firner
 *
 * State of an area in the game.
 */

#pragma once

#include <list>
#include <map>
#include <string>
#include <vector>

// Forward declare world state because it is used in Entity's dependencies.
struct WorldState;
#include "entity.hpp"

struct WorldEvent {
    std::string message;
    size_t y;
    size_t x;
};

struct WorldState {
    std::list<Entity> entities;
    std::map<std::string, decltype(entities)::iterator> named_entities;

    std::vector<WorldEvent> events;

    size_t field_height;
    size_t field_width;

    // The current time, in ticks. Advanced in the update function.
    size_t cur_tick = 0;

    // Keep track of what is passable.
    std::vector<std::vector<bool>> passable;

    bool isPassable(size_t y, size_t x);

    WorldState(size_t field_height, size_t field_width);
    void addEntity(size_t y, size_t x, const std::string& name, const std::set<std::string>& traits);

    // Returns true if the mob is moved, false otherwise.
    bool moveEntity(Entity& entity, size_t y, size_t x);

    // Damage entity_i for damage health points. Repercussions may happen to the attacker.
    void damageEntity(decltype(entities)::iterator entity_i, size_t damage, Entity& attacker);

    // Find the named entity, or named_entities.end()
    decltype(entities)::iterator findEntity(const std::string& name);

    // Find the named entity within the given range, or named_entities.end()
    decltype(entities)::iterator findEntity(const std::string& name, int64_t y, int64_t x, size_t range);

    // Find an entity with the given traits within the given range, or named_entities.end()
    decltype(entities)::iterator findEntity(const std::vector<std::string>& traits, int64_t y, int64_t x, size_t range);

    // Initialize layers, such as passable areas, and named entities.
    void initialize();

    // Log an event at the given location.
    void logEvent(WorldEvent event);

    // Fetch events with a given range of a coordinate.
    std::vector<std::string> getLocalEvents(size_t y, size_t x, size_t range);

    // Clear events
    void clearEvents();

    // Update layers and entities.
    void update();
};
