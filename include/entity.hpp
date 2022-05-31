/*
 * Copyright 2022 Bernhard Firner
 *
 * Entity in the game. Has a position, a name, and some traits.
 */

#pragma once

#include <atomic>
#include <functional>
#include <optional>
#include <map>
#include <set>
#include <string>
#include <vector>

// Need to forward declare Entity here since the class is used inside of the world state.
struct Entity;
#include "world_state.hpp"
#include "behavior.hpp"

struct Stats {
    // Physical
    size_t strength;
    size_t dexterity;
    size_t vitality;
    // Metaphysical
    size_t aura;
    size_t domain;
    size_t channel_rate;

    // Current status
    size_t health;
    size_t mana;
    size_t stamina;

    size_t species_level;
    size_t class1_level;
    size_t class2_level;
    size_t class3_level;

    void ticHealthManaStamina(size_t tick_num);

    // The maximum mana of this entity (derived from aura and domain)
    size_t maxMana() const;

    // The maximum health from these stats (a derived value)
    size_t maxHealth() const;

    // The maximum stamina from these stats (a derived value)
    size_t maxStamina() const;

    // The detection range of this entity (derived from stamina)
    size_t detectionRange() const;

    bool operator==(const Stats&) const = default;
};

struct Entity {
    // Every entity is created with a unique ID number.
    static std::atomic_size_t next_entity_id;
    size_t entity_id;
    // Location of the entity
    size_t y = 0;
    size_t x = 0;

    // Name of the entity
    std::string name;

    // Traits of this entity
    std::set<std::string> traits;

    // Optional stats. Generally only for non-objects.
    std::optional<Stats> stats;

    // Convenience function to find the species from the entity's traits
    // TODO Should this return optional<string> instead for safety?
    std::string getSpecies() const;

    // Rules that control how this entity should behave.
    std::string behavior_set_name;

    // The command handling functions of this entity.
    // The second argument, the argument list to the command, is documented in command_args.
    // These keep command handling tied to the entity level, while the commands themselves will be
    // enqueued in the command queue.
    // Notice that these will most likely be lambda functions with references to the entity that
    // they effect.
    std::map<std::string, std::function<void(WorldState&, const std::vector<std::string>&)>> command_handlers = {};
    std::map<std::string, Behavior::Ability> command_details = {};

    // Master of a command. Increases effectiveness and possibly unlocks new commands and behaviors.
    std::map<std::string, double> command_mastery = {};

    // Commands written into the essence core of the entity, enhancing their effectiveness with the
    // aura and domain attributes. Sometimes requires to achieve higher levels of master.
    std::vector<std::string> core_commands = {};

    // Constructors
    Entity(size_t y, size_t x, const std::string& name, const std::set<std::string> traits);

    Entity(const Entity&);

    std::string getDescription() const;

    // Equality operator. Based upon the entity_id value.
    bool operator==(const Entity&) const;
    bool operator==(const size_t) const;
};
