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
    // TODO Remove this in favor of getNamedEntry
    std::set<std::string> getHasA(const Entity& entity);
    std::set<std::string> getIsA(const Entity& entity);
    std::set<std::string> getLikes(const Entity& entity);
    std::set<std::string> getHates(const Entity& entity);
    // TODO Get other things like XP
};

