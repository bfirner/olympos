/*
 * Copyright 2022 Bernhard Firner
 *
 * Behavior handling for the game. Some behaviors unlock other behaviors once a certain mastery is
 * reached.
 * Contains functions to load behaviors from json configurations, functions to return available
 * commands from behaviors, functions to find behaviors that support given commands, and functions
 * to check the advancement of commands and behaviors.
 */

#pragma once

#include <functional>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "entity.hpp"
#include "world_state.hpp"

#include <nlohmann/json.hpp>


// TODO The commands generated from the behaviors need to affect entity states and the world state.
// Should they do it directly, or put effects onto another event queue?

namespace Behavior {

    // TODO FIXME Use NLOHMANN_JSON_SERIALIZE_ENUM for these enums.

    enum class AbilityType {
        unknown,
        movement,
        attack
    };

    enum class AbilityArea {
        unknown,
        single,
        line,
        cone,
        radius
    };

    enum class AbilityRange {
        unknown,
        close,
        medium,
        far
    };

    struct Ability {
        // Ability name
        std::string name;
        // TODO Description?

        // Ability type and area of effect.
        AbilityType type;
        AbilityArea area;
        AbilityRange range;
        std::vector<std::string> arguments;
        // Each effect may change multiple variables or require multiple variables to calculate.
        std::map<std::string, std::map<std::string, double>> effects;
        // Abilities that must be known prior to this one.
        std::map<std::string, size_t> prereqs;
        // Traits that must be possessed to use this ability.
        std::vector<std::string> constraints;
        // Flavor text when this ability is used.
        std::string flavor;

        std::function<void(WorldState&, const std::vector<std::string>&)> makeFunction(Entity& entity) const;

        // Construct from a json object.
        Ability(const std::string& name, nlohmann::json& ability_json);
    };

    struct BehaviorSet {
        // Name of the behavior set.
        std::string name;
        // Description of the behavior set.
        std::string description;

        // Abilities in the set.
        std::map<std::string, Ability> abilities;

        // Construct from a json object.
        BehaviorSet(const std::string& name, nlohmann::json& behavior_json);

        // Return which abilities from this set are available to an entity.
        std::vector<std::string> getAvailable(const Entity& entity) const;

        // Get the ability function for the given entity.
        std::function<void(WorldState&, const std::vector<std::string>&)> makeFunction(const std::string& ability, Entity& entity) const;

        // TODO A check if an entity can use the behavior set.
    };

    // Get all of the available behavior sets.
    const std::vector<BehaviorSet>& getBehaviors();
}

