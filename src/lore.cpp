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
#include <tuple>
#include <vector>

#include "lore.hpp"
#include "entity.hpp"

using json = nlohmann::json;

// Different species in the world.
json species;
// Objects in the world.
json objects;

std::mt19937 randgen{std::random_device{}()};

json& OlymposLore::getSpeciesLore() {
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

json& OlymposLore::getObjectLore() {
    // Read in the file if it hasn't already been done.
    const std::filesystem::path object_path{"resources/objects.json"};
    if (0 == species.size()) {
        if (std::filesystem::exists(object_path)) {
            std::ifstream istream(object_path.string(), std::ios::binary);
            if (istream) {
                std::string contents;
                std::getline(istream, contents, '\0');
                objects = json::parse(contents);
            }
        }
    }
    return objects;
}

std::tuple<json&, json&> getIsAHasA(const Entity& entity) {
    // A static object to return when there is no match.
    static json nothing{};
    std::string species_name = entity.getSpecies();
    std::string object_type = entity.getObjectType();
    // Check for species resolution first, then object resolution. This prioritizes the species
    // description of something that is both a creature and an object.
    if (species.contains(species_name)) {
        return {species.at(species_name).at("is a"), species.at(species_name).at("has a")};
    }
    else if (objects.contains(object_type)) {
        return {objects.at(object_type).at("is a"), objects.at(object_type).at("has a")};
    }
    else {
        return {nothing, nothing};
    }
}

std::string OlymposLore::getDescription(const Entity& entity) {
    json& species = getSpeciesLore();
    json& objects = getObjectLore();
    std::string species_name = entity.getSpecies();
    std::string object_type = entity.getObjectType();
    // Search for objects if this doesn't seem to be a species type.
    if (("" == species_name and "" == object_type) or
            not (species.contains(species_name) or objects.contains(object_type))) {
        return "Unknown entity.";
    }
    // Check for species resolution first, then object resolution. This prioritizes the species
    // description of something that is both a creature and an object.
    std::string description = species_name + ": ";
    if ("" == species_name) {
        description = object_type + ": ";
    }
    auto [json_is_a, json_has_a] = getIsAHasA(entity);
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
    json& species = getSpeciesLore();
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

    stats.channel_rate = std::floor(base["channel rate"].get<double>() + stats.species_level * growth["channel rate"].get<double>());
    stats.strength     = std::floor(base["strength"].get<double>() + stats.species_level * growth["strength"].get<double>());
    stats.reflexes    = std::floor(base["reflexes"].get<double>() + stats.species_level * growth["reflexes"].get<double>());
    stats.vitality     = std::floor(base["vitality"].get<double>() + stats.species_level * growth["vitality"].get<double>());
    stats.aura         = std::floor(base["aura"].get<double>() + stats.species_level * growth["aura"].get<double>());
    stats.domain       = std::floor(base["domain"].get<double>() + stats.species_level * growth["domain"].get<double>());

    return stats;

    // TODO Class attributes.
}

//TODO FIXME These functions are all almost exactly the same, maybe provide a generic function to
//fetch an entity from the json based upon a string.

std::set<std::string> OlymposLore::getLoreField(const std::string lore_name, const std::string& field) {
    json& species = getSpeciesLore();
    json& objects = getObjectLore();

    // Prepare a return set
    std::set<std::string> found_data;

    // Search for this name in species and objects
    if (species.contains(lore_name)) {
        auto& json_data = species[lore_name][field];
        found_data.insert(json_data.begin(), json_data.end());
    }
    if (objects.contains(lore_name)) {
        auto& json_data = objects[lore_name][field];
        found_data.insert(json_data.begin(), json_data.end());
    }
    // Now return whatever was found.
    return found_data;
}

std::string OlymposLore::getLoreString(const std::string lore_name, const std::string& field) {
    json& species = getSpeciesLore();
    json& objects = getObjectLore();

    // Search for this name in species and objects
    if (species.contains(lore_name)) {
        return species.at(lore_name).at(field).get<std::string>();
    }
    if (objects.contains(lore_name)) {
        return objects.at(lore_name).at(field).get<std::string>();
    }

    // If nothing was found then return an empty string.
    return "";
}

std::set<std::string> OlymposLore::getNamedEntry(const Entity& entity, const std::string& field) {
    std::set<std::string> species_fields = getLoreField(entity.getSpecies(), field);
    std::set<std::string> object_fields = getLoreField(entity.getObjectType(), field);

    species_fields.insert(object_fields.begin(), object_fields.end());

    return species_fields;
}

// TODO Get other things like XP
