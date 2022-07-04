/*
 * Copyright 2022 Bernhard Firner
 *
 * State of an area in the game.
 */

#pragma once

#include <deque>
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

class WorldState {
    private:
        void updatePassable(size_t y, size_t x);

        // The current time, in ticks. Advanced in the update function.
        size_t cur_tick = 0;

        // Transient events that occur with each tick of the world.
        std::vector<WorldEvent> events;
    public:
        std::list<Entity> entities;

        // Background colors representing effects.
        // TODO FIXME HERE Update in the behavior.cpp functions, pass to UserInterface::updateDisplay in
        // main, and clear in the update function.
        std::map<std::tuple<size_t, size_t>, std::string> background_effects;

        // Tick-persistent information and observations made by the player.
        std::deque<std::vector<std::wstring>> info_log;

        size_t field_height;
        size_t field_width;

        // Keep track of what is passable.
        std::vector<std::vector<bool>> passable;

        bool isPassable(size_t y, size_t x);

        WorldState(size_t field_height, size_t field_width);
        void addEntity(size_t y, size_t x, const std::string& name, const std::set<std::string>& traits);

        // Returns true if the mob is moved, false otherwise.
        bool moveEntity(Entity& entity, size_t y, size_t x);

        // Damage entity_i for damage health points. Repercussions may happen to the attacker.
        void damageEntity(decltype(entities)::iterator entity_i, size_t damage, Entity& attacker);

        // Find the named entity, or entities.end()
        decltype(entities)::iterator findEntity(const std::string& name);

        // Find an entity with the given traits, or entities.end()
        decltype(entities)::iterator findEntity(const std::vector<std::string>& traits);

        // Find the named entity within the given range, or entities.end()
        decltype(entities)::iterator findEntity(const std::string& name, int64_t y, int64_t x, size_t range);

        // Find an entity with the given traits within the given range, or entities.end()
        decltype(entities)::iterator findEntity(const std::vector<std::string>& traits, int64_t y, int64_t x, size_t range);

        // Find an entity with the given entity ID number.
        decltype(entities)::iterator findEntity(size_t entity_id);

        // Find all entities with the tiven traits within the given range.
        std::vector<decltype(entities)::iterator> findEntities(const std::vector<std::string>& traits, int64_t y, int64_t x, size_t range);

        // Initialize layers, such as passable areas, and named entities.
        void initialize();

        // Log information observed by the entity. Information remains present until cleared.
        void logInformation(const std::vector<std::wstring>& information);

        // Log an event at the given location.
        void logEvent(WorldEvent event);

        // Fetch events with a given range of a coordinate.
        std::vector<std::string> getLocalEvents(size_t y, size_t x, size_t range);

        // Clear events
        void clearEvents();

        // Update layers and entities.
        void update();
};
