/*
 * Copyright 2022 Bernhard Firner
 *
 * Information (growth and flavor text) about species and objects in the game.
 */

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <random>
#include <string>
#include <vector>

#include "lore.hpp"
#include "entity.hpp"

using json = nlohmann::json;

json species;

std::mt19937 randgen{std::random_device{}()};

json& getSpecies() {
    // Read in the file if it hasn't already been done.
    const std::filesystem::path species_path{"resources/species.json"};
    if (0 == species.size()) {
        if (std::filesystem::exists(species_path)) {
            std::ifstream istream(species_path.string(), std::ios::binary);
            if (istream) {
                std::string contents;
                std::getline(istream, contents, '\0');
                species = json::parse(contents);
            }
        }
    }
    return species;
}

std::string OlymposLore::getDescription(const Entity& entity) {
    json& species = getSpecies();
    std::string species_name = entity.getSpecies();
    // Don't try anything if there is no species name.
    if ("" == species_name or not species.contains(species_name)) {
        return "Unknown entity.";
    }
    std::string description = species_name + ": ";
    auto& json_is_a = species.at(species_name).at("is a");
    auto& json_has_a = species.at(species_name).at("has a");
    std::string is_a = "A mysterious entity";
    if (0 < json_is_a.size()) {
        std::uniform_int_distribution<int> uniform_dist(0, json_is_a.size()-1);
        is_a = json_is_a[uniform_dist(randgen)].get<std::string>();
    }
    std::string has_a = "";
    if (0 < json_has_a.size()) {
        std::uniform_int_distribution<int> uniform_dist(0, json_has_a.size()-1);
        has_a = " that has a " + json_has_a[uniform_dist(randgen)].get<std::string>();
        // We could do something like this, but it is a bit insane.
        // std::sample(json_is_a.begin(), json_is_a.end(), std::back_inserter(is_a), 1, randgen);
    }
    return is_a + has_a;
}

std::optional<Stats> OlymposLore::getStats(const Entity& entity) {
    json& species = getSpecies();
    std::string species_name = entity.getSpecies();
    // Don't try anything if there is no species name.
    if ("" == species_name or not species.contains(species_name)) {
        return {};
    }
    // Start from the existing stats, but if the entity doesn't already have stats then assume it
    // should start from level 1.
    Stats stats;
    if (entity.stats) {
        stats = entity.stats.value();
    }
    else {
        // TODO Is memory automatically cleaned? Is there a correct shorthand to make all values 0?
        stats = Stats{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
        stats.species_level = 1;
    }

    auto& base = species[species_name]["starting attributes"];
    auto& growth = species[species_name]["attribute growth"];

    stats.pool_volume  = std::floor(base["mana pool"].get<double>() + stats.species_level * growth["mana pool"].get<double>());
    stats.channel_rate = std::floor(base["channel rate"].get<double>() + stats.species_level * growth["channel rate"].get<double>());
    stats.strength     = std::floor(base["strength"].get<double>() + stats.species_level * growth["strength"].get<double>());
    stats.dexterity    = std::floor(base["dexterity"].get<double>() + stats.species_level * growth["dexterity"].get<double>());
    stats.vitality     = std::floor(base["vitality"].get<double>() + stats.species_level * growth["vitality"].get<double>());
    stats.aura         = std::floor(base["aura"].get<double>() + stats.species_level * growth["aura"].get<double>());
    stats.domain       = std::floor(base["domain"].get<double>() + stats.species_level * growth["domain"].get<double>());

    return stats;

    // TODO Class attributes.
}

//TODO FIXME These functions are all almost exactly the same, maybe provide a generic function to
//fetch an entity from the json based upon a string.

std::set<std::string> OlymposLore::getSpeciesField(const std::string species_name, const std::string& field) {
    json& species = getSpecies();
    // Don't try anything if there is no species name.
    if ("" == species_name or not species.contains(species_name)) {
        return std::set<std::string>{};
    }
    auto& json_data = species[species_name][field];
    return std::set<std::string>(json_data.begin(), json_data.end());
}

std::set<std::string> OlymposLore::getNamedEntry(const Entity& entity, const std::string& field) {
    json& species = getSpecies();
    std::string species_name = entity.getSpecies();
    // Don't try anything if there is no species name.
    if ("" == species_name or not species.contains(species_name)) {
        return std::set<std::string>{};
    }
    auto& json_data = species[species_name][field];
    return std::set<std::string>(json_data.begin(), json_data.end());
}

// TODO Get other things like XP
