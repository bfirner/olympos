/*
 * Copyright 2022 Bernhard Firner
 *
 * Information (growth and flavor text) about species in the game.
 */

#pragma once

#include <optional>
#include <set>
#include <string>

#include "entity.hpp"

namespace OlymposLore {
    std::string getDescription(const Entity& entity);
    std::optional<Stats> getStats(const Entity& entity);
    std::set<std::string> getNamedEntry(const Entity& entity, const std::string& field);
    std::set<std::string> getSpeciesField(const std::string species_name, const std::string& field);
    // TODO Remove this in favor of getNamedEntry
    // TODO Get other things like XP
};

