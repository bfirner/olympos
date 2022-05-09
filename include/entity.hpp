/*
 * Copyright 2022 Bernhard Firner
 *
 * Entity in the game. Has a position, a name, and some traits.
 */

#pragma once

#include <optional>
#include <set>
#include <string>

struct Stats {
    // Mana
    size_t pool_volume;
    size_t channel_rate;
    // Physical
    size_t power;
    size_t dexterity;
    size_t vitality;
    // Metaphysical
    size_t aura;
    size_t domain;

    // Current status
    // Max health is 1 + vitality*0.8 + domain*0.2
    size_t health;
    size_t mana;

    size_t species_level;
    size_t class1_level;
    size_t class2_level;
    size_t class3_level;
};

struct Entity {
    size_t y = 0;
    size_t x = 0;
    std::string name;

    std::set<std::string> traits;

    std::optional<Stats> stats;

    std::string getSpecies() const;
};
