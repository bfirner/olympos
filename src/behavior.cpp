/*
 * Copyright 2022 Bernhard Firner
 *
 * Behavior handling for the game. Some behaviors unlock other behaviors once a certain mastery is
 * reached.
 * Contains functions to load behaviors from json configurations, functions to return available
 * commands from behaviors, functions to find behaviors that support given commands, and functions
 * to check the advancement of commands and behaviors.
 */

#include <filesystem>
#include <fstream>
#include <random>

#include "behavior.hpp"

#include <nlohmann/json.hpp>

using json = nlohmann::json;

using std::vector;
using std::string;

namespace Behavior {

    json json_behaviors;

    std::vector<Behavior::BehaviorSet> loaded_behaviors;

    // No op function
    void noop_function(WorldState& ws, const vector<string>& args) {
        // Lots of nothing here.
        (ws);
        (args);
    }

    json& loadBehaviors() {
        // Read in the file if it hasn't already been done.
        const std::filesystem::path behaviors_path{"resources/behavior.json"};
        if (0 == json_behaviors.size()) {
            if (std::filesystem::exists(behaviors_path)) {
                std::ifstream istream(behaviors_path.string(), std::ios::binary);
                if (istream) {
                    std::string contents;
                    std::getline(istream, contents, '\0');
                    json_behaviors = json::parse(contents);
                }
            }
        }
        return json_behaviors;
    }

    AbilityType stoType(const std::string& str) {
        if (str == "movement") {
            return AbilityType::movement;
        }
        else if (str == "attack") {
            return AbilityType::attack;
        }
        return AbilityType::unknown;
    }

    AbilityArea stoArea(const std::string& str) {
        if (str == "single") {
            return AbilityArea::single;
        }
        else if (str == "line") {
            return AbilityArea::line;
        }
        else if (str == "cone") {
            return AbilityArea::cone;
        }
        else if (str == "radius") {
            return AbilityArea::radius;
        }
        return AbilityArea::unknown;
    }

    AbilityRange stoRange(const std::string& str) {
        if (str == "close") {
            return AbilityRange::close;
        }
        else if (str == "medium") {
            return AbilityRange::medium;
        }
        else if (str == "far") {
            return AbilityRange::far;
        }
        return AbilityRange::unknown;
    }

    Behavior::Ability::Ability(const std::string& name, nlohmann::json& ability_json) {
        this->name = name;
        type = stoType(ability_json.at("type").get<string>());
        area = stoArea(ability_json.at("area").get<string>());
        range = stoRange(ability_json.at("range").get<string>());
        flavor = ability_json.at("flavor").get<string>();

        arguments = ability_json.at("arguments").get<vector<string>>();
        effects = ability_json.at("effects").get<decltype(effects)>();
        prereqs = ability_json.at("prereqs").get<decltype(prereqs)>();
        constraints = ability_json.at("constraints").get<decltype(constraints)>();
    }

    Behavior::BehaviorSet::BehaviorSet(const std::string& name, nlohmann::json& behavior_json) {
        this->name = name;
        description = behavior_json.at("description");

        abilities = std::map<std::string, Ability>();

        for (auto& [ability_name, ability_json] : behavior_json.at("abilities").get<std::map<std::string, json>>()) {
            abilities.insert({ability_name, Ability(ability_name, ability_json)});
        }
    }

    const std::vector<BehaviorSet>& getBehaviors() {
        if (0 < loaded_behaviors.size()) {
            return loaded_behaviors;
        }
        // Otherwise we need to load the json file and populate the behaviors.
        json& behaviors = loadBehaviors();
        // Go through the json and translate all of the entries into new BehaviorSets.
        for (auto& [behavior_name, behavior_json] : behaviors.get<std::map<std::string, json>>()) {
            loaded_behaviors.push_back(BehaviorSet(behavior_name, behavior_json));
        }
        return loaded_behaviors;
    }

    // Make a function for the specific entity.
    std::function<void(WorldState&, const std::vector<std::string>&)> Behavior::Ability::makeFunction(Entity& entity) const {
        if (type == AbilityType::movement) {
            if (effects.contains("distance")) {
                auto& distances = effects.at("distance");
                if (distances.contains("x") or distances.contains("y")) {
                    int x_dist = 0;
                    int y_dist = 0;
                    if (distances.contains("x")) {
                        x_dist = distances.at("x");
                    }
                    if (distances.contains("y")) {
                        y_dist = distances.at("y");
                    }
                    // Capture the distances by value, but reference the entity.
                    return [=,&entity](WorldState& ws, const vector<string>& args) {
                        // Ignoring the movement arguments
                        (args);
                        if (ws.passable[entity.y+y_dist][entity.x+x_dist]) {
                            entity.x += x_dist;
                            entity.y += y_dist;
                        }
                    };
                }
                else if (distances.contains("random_min") and distances.contains("random_max")) {
                    // Create a random generator in the given range.
                    int rand_min = distances.at("random_min");
                    int rand_max = distances.at("random_max");

                    return [=,&entity](WorldState& ws, const vector<string>& args) {
                        // Ignoring the movement arguments
                        // Going to use one RNG for each entity. This theoretically protects from
                        // some side channel shenanigans.
                        static std::mt19937 randgen{std::random_device{}()};
                        static std::uniform_int_distribution<> rand_direction(0, 1);
                        static std::uniform_int_distribution<> rand_distance(rand_min, rand_max);
                        (args);
                        int x_dist = 0;
                        int y_dist = 0;
                        if (0 == rand_direction(randgen)) {
                            x_dist += rand_distance(randgen);
                        }
                        else {
                            y_dist += rand_distance(randgen);
                        }
                        if (ws.passable[entity.y+y_dist][entity.x+x_dist]) {
                            entity.x += x_dist;
                            entity.y += y_dist;
                        }
                    };
                }
            }
        }
        // TODO Otherwise return a nothing
        return noop_function;
    }

    // Find the ability names that are available to this entity within this behavior set.
    std::vector<std::string> BehaviorSet::getAvailable(const Entity& entity) const {
        // TODO Check if ability set should be available
        std::vector<std::string> available;

        for (auto& [ability_name, ability] : abilities) {
            bool can_use = true;
            // TODO prereqs
            // Verify that the entity satisfies all constraints
            if (0 < ability.constraints.size()) {
                if ("or" == ability.constraints.front()) {
                    can_use = can_use and std::any_of(ability.constraints.begin()+1, ability.constraints.end(),
                        [&](const std::string& constraint) {return entity.traits.contains(constraint);});
                }
                else {
                    can_use = can_use and std::all_of(ability.constraints.begin(), ability.constraints.end(),
                        [&](const std::string& constraint) {return entity.traits.contains(constraint);});
                }
            }
            if (can_use) {
                available.push_back(ability_name);
            }
        }
        return available;
    }

    // Update abilities from this set that are available to an entity. Return new abilities.
    std::vector<std::string> BehaviorSet::updateAvailable(Entity& entity) const {
        // TODO Check if ability set should be available
        std::vector<std::string> available;

        for (auto& [ability_name, ability] : abilities) {
            bool can_use = true;
            // TODO prereqs
            // Verify that the entity satisfies all constraints
            if (0 < ability.constraints.size()) {
                if ("or" == ability.constraints.front()) {
                    can_use = can_use and std::any_of(ability.constraints.begin()+1, ability.constraints.end(),
                        [&](const std::string& constraint) {return entity.traits.contains(constraint);});
                }
                else {
                    can_use = can_use and std::all_of(ability.constraints.begin(), ability.constraints.end(),
                        [&](const std::string& constraint) {return entity.traits.contains(constraint);});
                }
            }
            if (can_use) {
                entity.command_handlers.insert({ability_name, makeFunction(ability_name, entity)});
                available.push_back(ability_name);
            }
        }
        return available;
    }


    std::function<void(WorldState&, const std::vector<std::string>&)> BehaviorSet::makeFunction(const std::string& ability, Entity& entity) const {
        // Make the function for this ability.
        if (not abilities.contains(ability)) {
            return noop_function;
        }
        return abilities.at(ability).makeFunction(entity);
    }

}
