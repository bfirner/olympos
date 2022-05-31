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
#include <tuple>
#include <vector>

// Forward declare types
namespace Behavior {
    struct Ability;
    struct AbilitySet;
    struct BehaviorSet;
}

#include "command_handler.hpp"
#include "entity.hpp"
#include "world_state.hpp"

#include <nlohmann/json.hpp>


// TODO The commands generated from the behaviors need to affect entity states and the world state.
// Should they do it directly, or put effects onto another event queue?

namespace Behavior {

    // TODO FIXME Split behavior stuff and ability stuff into two different files.

    enum class AbilityType {
        unknown,
        movement,
        attack
    };

    NLOHMANN_JSON_SERIALIZE_ENUM(
        AbilityType,
        {
            {AbilityType::unknown, nullptr},
            {AbilityType::movement, "movement"},
            {AbilityType::attack, "attack"},
        })

    enum class AbilityArea {
        unknown,
        single,
        line,
        cone,
        radius
    };

    NLOHMANN_JSON_SERIALIZE_ENUM(
        AbilityArea,
        {
            {AbilityArea::unknown, nullptr},
            {AbilityArea::single, "single"},
            {AbilityArea::line, "line"},
            {AbilityArea::cone, "cone"},
            {AbilityArea::radius, "radius"},
        })

    enum class AbilityRange {
        unknown,
        close,
        medium,
        far
    };

    NLOHMANN_JSON_SERIALIZE_ENUM(
        AbilityRange,
        {
            {AbilityRange::unknown, nullptr},
            {AbilityRange::close, "close"},
            {AbilityRange::medium, "medium"},
            {AbilityRange::far, "far"},
        })

    struct Ability {
        // Ability name
        std::string name;
        // TODO Description?

        // Ability type and area of effect.
        AbilityType type;
        AbilityArea area;
        AbilityRange range;
        size_t stamina;
        std::vector<std::string> arguments;
        std::vector<std::string> default_args;
        // Each effect may change multiple variables or require multiple variables to calculate.
        std::map<std::string, nlohmann::json> effects;
        // Abilities that must be known prior to this one.
        std::map<std::string, size_t> prereqs;
        // Traits that must be possessed to use this ability.
        std::vector<std::string> constraints;
        // Flavor text when this ability is used.
        std::string flavor;

        // TODO FIXME If these are being handled separately anyway, then the Ability class should be
        // subclassed for each of them anyway and a factory can decide which one to create.
        // TODO FIXME Should these just be objects instead of lambda functions? The lambda functions
        // can't be queried for information, which is annoying.
        // Make a movement type of function.
        std::function<void(WorldState&, const std::vector<std::string>&)> makeMoveFunction(Entity& entity) const;
        std::function<void(WorldState&, const std::vector<std::string>&)> makeConditionalMoveFunction(Entity& entity) const;
        std::function<void(WorldState&, const std::vector<std::string>&)> makeLinearMoveFunction(Entity& entity) const;

        // Make an attack type of function.
        std::function<void(WorldState&, const std::vector<std::string>&)> makeAttackFunction(Entity& entity) const;

        // Make a function for this ability
        std::function<void(WorldState&, const std::vector<std::string>&)> makeFunction(Entity& entity) const;

        // Construct from a json object.
        Ability(const std::string& name, nlohmann::json& ability_json);
    };

    struct AbilitySet {
        // Name of the ability set.
        std::string name;
        // Description of the ability set.
        std::string description;

        // Abilities in the set.
        std::map<std::string, Ability> abilities;

        // Construct from a json object.
        AbilitySet(const std::string& name, nlohmann::json& behavior_json);

        // Return which abilities from this set are available to an entity.
        std::vector<std::string> getAvailable(const Entity& entity) const;

        // Update abilities from this set that are available to an entity. Return new abilities.
        std::vector<std::string> updateAvailable(Entity& entity) const;

        // Get the ability function for the given entity.
        std::function<void(WorldState&, const std::vector<std::string>&)> makeFunction(const std::string& ability, Entity& entity) const;

        // TODO A check if an entity can use the behavior set.
    };

    // Get all of the available behavior sets.
    const std::vector<AbilitySet>& getAbilities();

    // Conditions and abilities that define the behavior of an entity.
    struct BehaviorSet {
        // Name of the behavior set.
        std::string name;

        // Description
        std::string description;

        // Conditions and actions that make up this behavior set, ordered by precedence.
        std::vector<std::vector<std::string>> rules;

        // Go through the behavior set of the given entity and follow its rules to take appropriate
        // actions.
        void executeBehavior(Entity&, WorldState&, CommandHandler&) const;
    };

    const std::map<std::string, BehaviorSet>& getBehaviors();
}

