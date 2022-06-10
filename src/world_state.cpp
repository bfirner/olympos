/*
 * Copyright 2022 Bernhard Firner
 *
 * State of an area in the game.
 */

#include <algorithm>
#include <cmath>
#include <execution>
#include <functional>
#include <set>
#include <optional>
#include <regex>
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
void updatePassable(const std::list<Entity>& entities, vector<vector<bool>>& passable) {
    // Set everything passable and then update things that are not.
    for (auto& row : passable) {
        row.assign(true, row.size());
    }
    for (auto& entity_p : entities) {
        if (not isPassable(entity_p)) {
            passable[entity_p.y][entity_p.x] = false;
        }
    }
}


bool WorldState::isPassable(size_t y, size_t x) {
    // Out of bounds? Return false.
    if (y >= this->field_height or x >= this->field_width) {
        return false;
    }
    // Return false if given location is not passable.
    if (not passable[y][x]) {
        return false;
    }
    // Otherwise return true.
    return true;
}

WorldState::WorldState(size_t field_height, size_t field_width) :
    passable{field_height, vector<bool>(field_width, true)} {
    this->field_height = field_height;
    this->field_width = field_width;
}

void WorldState::addEntity(size_t y, size_t x, const std::string& name, const std::set<std::string>& traits) {
    if (y >= this->field_height or x >= this->field_width) {
        throw std::runtime_error("Cannot place entity at "+std::to_string(y)+", "+std::to_string(x)+": out of bounds.");
    }
    // TODO FIXME Make a real constructor for the Entity class
    entities.push_front(Entity(y, x, name, traits));

    // Calculate starting stats for this entity (if it has any)
    /*
    std::optional<Stats> stats = OlymposLore::getStats(entities.front());
    if (stats) {
        entities.front().stats = stats.value();
    }
    */
}

bool passableOrNotPresent(size_t y, size_t x, const Entity& ent) {
    return ent.y != y or ent.x != x or isPassable(ent);
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

    // Move the entity to the new location
    size_t old_y = entity.y;
    size_t old_x = entity.x;
    entity.y = y;
    entity.x = x;

    // Update passable with this entity removed.
    passable[old_y][old_x] = std::all_of(
        std::execution::par_unseq, entities.begin(), entities.end(),
        std::bind_front(passableOrNotPresent, old_y, old_x));

    // Now updated passable at the new location. Note that we are calling the function to prepare
    // for the possibility more complex rules in the future.
    passable[y][x] = std::all_of(
        std::execution::par_unseq, entities.begin(), entities.end(),
        std::bind_front(passableOrNotPresent, y, x));
    return true;
}

void WorldState::damageEntity(decltype(entities)::iterator entity_i, size_t damage, Entity& attacker) {
    // Out of bounds, nothing happens.
    if (entities.end() == entity_i) {
        return;
    }

    if (entity_i->stats) {
        Stats& stats = entity_i->stats.value();
        if (damage >= stats.health) {
            stats.health = 0;
            // Remove the entity.
            // TODO FIXME Remove all of its actions from the action queue.
            // That implies that the action queue should be part of the world model.
            // Makes sense, the action log should live in the world model as well.
            entities.erase(entity_i);
        }
        else {
            stats.health -= damage;
        }
    }
}

std::list<Entity>::iterator WorldState::findEntity(const std::string& name) {
    std::regex pattern(name, std::regex_constants::icase);
    return std::find_if(entities.begin(), entities.end(),
        [&](Entity& ent) {return std::regex_search(ent.name, pattern);});
}

std::list<Entity>::iterator WorldState::findEntity(const std::vector<std::string>& traits) {
    return std::find_if(entities.begin(), entities.end(),
        [&](Entity& ent) {return std::all_of(traits.begin(), traits.end(), [&](const std::string& trait) {return ent.traits.contains(trait);});});
}

std::list<Entity>::iterator WorldState::findEntity(const std::string& name, int64_t y, int64_t x, size_t range) {
    std::regex pattern(name, std::regex_constants::icase);
    return std::find_if(entities.begin(), entities.end(),
        [&](Entity& ent) {return std::regex_search(ent.name, pattern) and (abs(y - ent.y) + abs(x - ent.x)) <= range;});
}

std::list<Entity>::iterator WorldState::findEntity(const std::vector<std::string>& traits, int64_t y, int64_t x, size_t range) {
    return std::find_if(entities.begin(), entities.end(),
        [&](Entity& ent) {return std::all_of(traits.begin(), traits.end(), [&](const std::string& trait) {return ent.traits.contains(trait);}) and (abs(y - ent.y) + abs(x - ent.x)) <= range;});
}

std::list<Entity>::iterator WorldState::findEntity(size_t entity_id) {
    return std::find(entities.begin(), entities.end(), entity_id);
}

void WorldState::initialize() {
    // Get the player entity
    decltype(entities)::iterator player = std::find_if(entities.begin(), entities.end(),
            [](auto& ent) {return (ent.traits.contains("player"));});

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
        if (entity_p.stats) {
            Stats& stats = entity_p.stats.value();
            stats.health = stats.maxHealth();
            stats.mana = stats.maxMana();
            stats.stamina = stats.maxStamina();
        }
    }

    updatePassable(entities, passable);
}

void WorldState::logInformation(const std::vector<std::wstring>& information) {
    info_log.push_front(information);
    if (3 < info_log.size()) {
        info_log.pop_back();
    }
}

void WorldState::logEvent(WorldEvent event) {
    if (event.y >= this->field_height or event.x >= this->field_width) {
        throw std::runtime_error("Cannot log event at "+std::to_string(event.y)+", "+std::to_string(event.x)+": out of bounds.");
    }
    events.push_back(event);
}

std::vector<std::string> WorldState::getLocalEvents(size_t y, size_t x, size_t range) {
    std::vector<std::string> local_events;
    for (WorldEvent& event : events) {
        if (abs(y - event.y) + abs(x - event.x) <= range) {
            // Making this a coroutine is possible, but current feels more clunky than it is worth.
            local_events.push_back(event.message);
        }
    }
    return local_events;
}

void WorldState::clearEvents() {
    events.clear();
}

void WorldState::update() {
    cur_tick += 1;
    // Need to handle events that occur every tick.

    // Tic updates are independent per entity and can be done in parallel and in any order.
    std::for_each(std::execution::par_unseq, entities.begin(), entities.end(),
        [cur_tick=cur_tick](Entity& ent) {
            if (ent.stats) {
            ent.stats.value().ticHealthManaStamina(cur_tick);
        }});
    // TODO FIXME The event queue should be handled a bit differently
    auto player_i = findEntity(std::vector<std::string>{"player"});
    if (player_i != entities.end()) {
        logEvent({"==========Tick " + std::to_string(cur_tick) + "========", player_i->y, player_i->x});
    }
}
