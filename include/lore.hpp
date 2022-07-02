/*
 * Copyright 2022 Bernhard Firner
 *
 * Information (growth and flavor text) about species in the game.
 */

#pragma once

#include <nlohmann/json.hpp>

#include <optional>
#include <set>
#include <string>

#include "entity.hpp"

using json = nlohmann::json;

namespace OlymposLore {
    std::string getDescription(const Entity& entity);
    std::optional<Stats> getStats(const Entity& entity);
    std::set<std::string> getNamedEntry(const Entity& entity, const std::string& field);
    std::set<std::string> getLoreField(const std::string lore_name, const std::string& field);
    std::string getLoreString(const std::string lore_name, const std::string& field);
    std::set<std::string> getSpecies(const std::string species_name, const std::string& field);
    // TODO Get other things like XP

    json& getSpeciesLore();
    json& getObjectLore();

    template<typename T>
    T getSpeciesData(const std::string species_name, const std::string& field) {
        json& species = getSpeciesLore();
        // Don't try anything if there is no species name.
        if ("" == species_name or not species.contains(species_name)) {
            return T{};
        }
        return species.at(species_name).at(field).get<T>();
    }
}

