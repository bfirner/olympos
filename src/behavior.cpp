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
#include <limits>
#include <random>
#include <ranges>

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
        // TODO Use NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE macro to automatically define the json to
        // ability conversion?
        this->name = name;
        type = ability_json.at("type").get<AbilityType>();
        //type = stoType(ability_json.at("type").get<string>());
        area = stoArea(ability_json.at("area").get<string>());
        range = stoRange(ability_json.at("range").get<string>());
        ability_json.at("flavor").get_to(flavor);

        ability_json.at("stamina").get_to(stamina);

        ability_json.at("arguments").get_to(arguments);
        if (ability_json.contains("default arguments")) {
            ability_json.at("default arguments").get_to(default_args);
        }
        ability_json.at("effects").get_to(effects);
        ability_json.at("prereqs").get_to(prereqs);
        ability_json.at("constraints").get_to(constraints);
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

    void replaceSubstring(std::string& str, const std::string& target, const std::string& replacement) {
        std::string::size_type index = str.find(target);
        if (index != std::string::npos) {
            str.replace(index, index+target.size(), replacement);
        }
    }

    // A function meant for binding that increases or decreases one entity's distance from another.
    void changeDistance(Entity& actor, const Ability& ability, std::string event_string, size_t desired_distance, WorldState& ws, const std::vector<std::string>& arguments) {
        // Verify that the actor can take this action
        if (ability.stamina > actor.stats.value().stamina) {
            return;
        }

        // Find the target from the arguments.
        if (arguments.size() < 1) {
            return;
        }
        const std::string& target_name = arguments.at(0);

        // First, check if the actor can see or sense the target.
        size_t detection_range = 0;
        if (actor.stats) {
            detection_range = actor.stats.value().detectionRange();
        }
        auto target_i = ws.findEntity(target_name, actor.y, actor.x, detection_range);

        // If the target was not found then take no action.
        if (target_i == ws.entities.end()) {
            // Check to see if this is a trait rather than a name.
            target_i = ws.findEntity(std::vector<string>{target_name}, actor.y, actor.x, detection_range);
            if (target_i == ws.entities.end()) {
                return;
            }
        }

        // If the target entity was found then calculate how to move closer or farther.
        // There won't be any pathing, this is just a straight line calculation.
        int y_dist = (int)actor.y - target_i->y;
        int x_dist = (int)actor.x - target_i->x;
        size_t distance = abs(y_dist) + abs(x_dist);
        // See if this entity wants to move closer or further
        size_t next_y_location = actor.y;
        size_t next_x_location = actor.x;
        // Move closer?
        if (distance > desired_distance) {
            // Determine the sign of movement and check if that space is passable.
            bool y_move_possible = (0 < y_dist and ws.isPassable(actor.y-1, actor.x)) or
                                   (0 > y_dist and ws.isPassable(actor.y+1, actor.x));
            bool x_move_possible = (0 < x_dist and ws.isPassable(actor.y, actor.x-1)) or
                                   (0 > x_dist and ws.isPassable(actor.y, actor.x+1));
            // Move closer along the farther dimension
            if (abs(y_dist) >= abs(x_dist) and y_move_possible) {
                if (0 < y_dist) {
                    next_y_location -= 1;
                }
                else if (0 > y_dist) {
                    next_y_location += 1;
                }
            }
            // If we didn't move in y, then try to move in x.
            else if (x_move_possible) {
                if (0 < x_dist) {
                    next_x_location -= 1;
                }
                else if (0 > x_dist) {
                    next_x_location += 1;
                }
            }
        }
        // Move farther
        else if (distance < desired_distance) {
            // Determine the sign of movement and check if that space is passable.
            bool y_move_possible = (0 <= y_dist and ws.isPassable(actor.y+1, actor.x)) or
                                   (0 >= y_dist and ws.isPassable(actor.y-1, actor.x));
            bool x_move_possible = (0 <= x_dist and ws.isPassable(actor.y, actor.x+1)) or
                                   (0 >= x_dist and ws.isPassable(actor.y, actor.x-1));
            // Move farther along the smaller dimension. This becomes ambiguous when the smaller
            // dimension has a distance of 0, in which case we break towards the positive direction
            // if it is possible to move to that location.
            if (abs(y_dist) <= abs(x_dist) and y_move_possible) {
                if (0 <= y_dist and ws.isPassable(actor.y+1, actor.x)) {
                    next_y_location += 1;
                }
                else if (0 >= y_dist) {
                    next_y_location -= 1;
                }
            }
            // If we didn't move in y, then try to move in x.
            else if (x_move_possible) {
                if (0 <= x_dist and ws.isPassable(actor.y, actor.x+1)) {
                    next_x_location += 1;
                }
                else if (0 >= x_dist) {
                    next_x_location -= 1;
                }
            }
        }
        // If there is movement and the actor has the stamina for the action take it, and then
        // reduce the stamina cost from the actor's stamina if the action occurred.
        if ((next_y_location != actor.y or next_x_location != actor.x) ) {
            if (ws.moveEntity(actor, next_y_location, next_x_location)) {
                replaceSubstring(event_string, "<target>", target_i->name);
                actor.stats.value().stamina -= ability.stamina;
                // TODO Event string setup on the calling side
                ws.logEvent({event_string, actor.y, actor.x});
            }
        }
    }

    std::function<void(WorldState&, const std::vector<std::string>&)> Ability::makeConditionalMoveFunction(Entity& entity) const {
        // Prepare a flavor string to go into the event log whenever this behavior occurs.
        // Fill in some fields in advance.
        std::string event_string = flavor;
        replaceSubstring(event_string, "<entity>", entity.name);

        if (effects.contains("minimize distance") and arguments.at(0) == "<target>") {
            return std::bind_front(changeDistance, std::ref(entity), std::cref(*this), event_string, 0);
        }
        else if (effects.contains("maximize distance") and arguments.at(0) == "<target>") {
            return std::bind_front(changeDistance, std::ref(entity), std::cref(*this), event_string, std::numeric_limits<size_t>::max()/2);
        }
        else if (effects.contains("maintain distance") and
                 arguments == vector<string>{"<target>", "range"}) {
            auto bound_fun = std::bind_front(changeDistance, std::ref(entity), std::cref(*this), event_string);
            return [=](WorldState& ws, const std::vector<std::string>& args) {
                size_t desired_distance = std::stoull(args.at(1));
                return bound_fun(desired_distance, ws, args);
            };
        }
        // Otherwise return a nothing
        return noop_function;
    }

    std::function<void(WorldState&, const std::vector<std::string>&)> Ability::makeLinearMoveFunction(Entity& entity) const {
        // Prepare a flavor string to go into the event log whenever this behavior occurs.
        // Fill in some fields in advance.
        std::string event_string = flavor;
        replaceSubstring(event_string, "<entity>", entity.name);

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
                // If the entity has the stamina for the action take it, and then reduce the
                // stamina cost from the entity's stamina if the action occurred.
                if (stamina <= entity.stats.value().stamina) {
                    if (ws.moveEntity(entity, entity.y + y_dist, entity.x + x_dist)) {
                        entity.stats.value().stamina -= stamina;
                        ws.logEvent({event_string, entity.y, entity.x});
                    }
                }
            };
        }
        else if (distances.contains("random_min") and distances.contains("random_max")) {
            // Create a random generator in the given range.
            int rand_min = distances.at("random_min");
            int rand_max = distances.at("random_max");

            // Lambda functions do not capture member variables, so shadow stamina with a
            // local variable.
            return [=,&entity,stamina=this->stamina](WorldState& ws, const vector<string>& args) {
                // Ignoring the movement arguments
                // Going to use one RNG for each lambda. This theoretically protects from
                // some side channel shenanigans.
                static std::mt19937 randgen{std::random_device{}()};
                static std::uniform_int_distribution<> rand_direction(0, 1);
                static std::uniform_int_distribution<> rand_distance(rand_min, rand_max);
                (args);
                int y_location = entity.y;
                int x_location = entity.x;
                // Chose a random movement
                if (0 == rand_direction(randgen)) {
                    y_location += rand_distance(randgen);
                }
                else {
                    x_location += rand_distance(randgen);
                }
                // If the entity has the stamina for the action take it, and then reduce the
                // stamina cost from the entity's stamina if the action occurred.
                if (stamina <= entity.stats.value().stamina) {
                    if (ws.moveEntity(entity, y_location, x_location)) {
                        entity.stats.value().stamina -= stamina;
                        ws.logEvent({event_string, entity.y, entity.x});
                    }
                }
            };
        }
        // Otherwise return a nothing
        return noop_function;
    }

    std::function<void(WorldState&, const std::vector<std::string>&)> Ability::makeMoveFunction(Entity& entity) const {
        if (effects.contains("distance")) {
            // Linear movement function
            return makeLinearMoveFunction(entity);
        }
        else {
            // Conditional movement function.
            return makeConditionalMoveFunction(entity);

        }
        // Otherwise return a nothing
        return noop_function;
    }

    std::function<void(WorldState&, const std::vector<std::string>&)> Ability::makeAttackFunction(Entity& entity) const {
        // Lambda functions do not capture member variables, so shadow stamina with a
        // local variable.
        double base = 0;
        double strength = 0;
        double domain = 0;
        double aura = 0;
        double dexterity = 0;
        if (effects.contains("damage")) {
            auto& damage_effects = effects.at("damage");
            if (damage_effects.contains("base")) {
                base = damage_effects.at("base");
            }
            if (damage_effects.contains("strength")) {
                strength = damage_effects.at("strength");
            }
            if (damage_effects.contains("domain")) {
                domain = damage_effects.at("domain");
            }
            if (damage_effects.contains("aura")) {
                aura = damage_effects.at("aura");
            }
            if (damage_effects.contains("dexterity")) {
                dexterity = damage_effects.at("dexterity");
            }
        }
        // TODO Or maybe just copy over any effects that are also in the expected arguments?
        std::vector<std::string> expected_args = arguments;
        std::vector<std::string> default_args = this->default_args;
        // TODO Make different classes for range and area combinations
        // Prepare a flavor string to go into the event log whenever this behavior occurs.
        // Fill in some fields in advance.
        std::string event_string = flavor;
        replaceSubstring(event_string, "<entity>", entity.name);

        return [=,&entity,effects=this->effects,stamina=this->stamina](WorldState& ws, const vector<string>& args) {
            size_t damage = floor(base + strength * entity.stats.value().strength + domain * entity.stats.value().domain +
                aura * entity.stats.value().aura + dexterity * entity.stats.value().dexterity);
            // Now parse the arguments to see what is getting hit.
            auto target = ws.entities.end();
            if (0 < expected_args.size() and expected_args[0] == "or") {
                // Expecting a single argument
                std::string arg = default_args.at(0);
                if (0 < args.size()) {
                    arg = args.at(0);
                }
                auto arg_options = std::ranges::subrange(expected_args.begin()+1, expected_args.end());
                // See if we match an argument
                if (arg_options.end() != std::find(arg_options.begin(), arg_options.end(), arg)) {
                    // TODO FIXME What about the forward option?
                    // Find the effects of for this argument.
                    if (effects.contains(arg)) {
                        if (effects.at(arg).contains("distance")) {
                            size_t target_y = entity.y;
                            size_t target_x = entity.x;
                            auto& distances = effects.at(arg).at("distance");
                            if (distances.contains("y")) {
                                target_y += distances.at("y").get<int>();
                            }
                            if (distances.contains("x")) {
                                target_x += distances.at("x").get<int>();
                            }
                            // Now find the target at that location if one exists.
                            // It is possible that there are multiple entities in that tile. This
                            // will find the first one arbitrarily.
                            target = std::find_if(ws.entities.begin(), ws.entities.end(),
                                [=](Entity& ent) { return ent.y == target_y and ent.x == target_x;});
                        }
                    }
                }
                else if (expected_args.end() != std::find(expected_args.begin(), expected_args.end(), ("<target>"))) {
                    // Going to have to search for this target in range.
                    std::string target_name = "";
                    if (0 < args.size()) {
                        target_name = args[0];
                    }
                    else if (0 < default_args.size()) {
                        target_name = default_args[0];
                    }
                    // Assign the target.
                    target = ws.findEntity(target_name);
                }
            }
            if (target != ws.entities.end()) {
                std::string log_string = event_string;
                replaceSubstring(log_string, "<target>", target->name);
                ws.logEvent({log_string, target->y, target->x});

                // Deal damage to the target
                ws.damageEntity(target, damage, entity);
            }
            // The attack always consumes stamina
            entity.stats.value().stamina -= stamina;
        };
        // Otherwise return a nothing
        return noop_function;
    }

    // Make a function for the specific entity.
    std::function<void(WorldState&, const std::vector<std::string>&)> Ability::makeFunction(Entity& entity) const {
        if (type == AbilityType::movement) {
            return makeMoveFunction(entity);
        }
        else if (type == AbilityType::attack) {
            return makeAttackFunction(entity);
        }
        // Otherwise return a nothing
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
                entity.command_details.insert({ability_name, ability});
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
