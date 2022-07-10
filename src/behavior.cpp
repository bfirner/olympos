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
#include <iostream>
#include <limits>
#include <random>
#include <ranges>
#include <regex>
#include <tuple>

#include "behavior.hpp"

#include <nlohmann/json.hpp>

using json = nlohmann::json;

using std::vector;
using std::string;

namespace Behavior {

    std::vector<Behavior::AbilitySet> loaded_abilities;
    std::map<std::string, BehaviorSet> loaded_behaviors;

    // No op function
    void noop_function(WorldState&, const vector<string>&) {
        // Lots of nothing here.
    }

    json loadJson(const std::string& path) {
        // Read in the file if it hasn't already been done.
        const std::filesystem::path json_path{path};
        json json_data;
        if (std::filesystem::exists(json_path)) {
            std::ifstream istream(json_path.string(), std::ios::binary);
            if (istream) {
                std::string contents;
                std::getline(istream, contents, '\0');
                json_data = json::parse(contents);
            }
        }
        else {
            std::cerr<<"Error finding file at "<<path<<'\n';
        }
        return json_data;
    }

    AbilityType stoType(const std::string& str) {
        if (str == "movement") {
            return AbilityType::movement;
        }
        else if (str == "attack") {
            return AbilityType::attack;
        }
        else if (str == "utility") {
            return AbilityType::utility;
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
        area = stoArea(ability_json.at("area").get<string>());
        range = stoRange(ability_json.at("range").get<string>());
        ability_json.at("flavor").get_to(flavor);
        ability_json.at("fail_flavor").get_to(fail_flavor);

        ability_json.at("stamina").get_to(stamina);

        ability_json.at("arguments").get_to(arguments);
        if (ability_json.contains("default arguments")) {
            ability_json.at("default arguments").get_to(default_args);
        }
        ability_json.at("effects").get_to(effects);
        ability_json.at("prereqs").get_to(prereqs);
        ability_json.at("constraints").get_to(constraints);
    }

    Behavior::AbilitySet::AbilitySet(const std::string& name, nlohmann::json& behavior_json) {
        this->name = name;
        description = behavior_json.at("description");

        abilities = std::map<std::string, Ability>();

        for (auto& [ability_name, ability_json] : behavior_json.at("abilities").get<std::map<std::string, json>>()) {
            abilities.insert({ability_name, Ability(ability_name, ability_json)});
        }
    }

    void replaceSubstring(std::string& str, const std::string& target, const std::string& replacement) {
        std::string::size_type index = str.find(target);
        if (index != std::string::npos) {
            str.replace(index, index+target.size(), replacement);
        }
    }

    bool actionBoilerplateCheck(Entity& actor, WorldState& ws, const Ability& ability, const std::vector<std::string>& arguments, size_t min_arguments, const std::string& fail_string) {
        // Verify that the actor can take this action
        if (ability.stamina > actor.stats.value().stamina) {
            // TODO Should there be a generic low stamina failure string?
            // TODO Prepend an exhausted string to the fail string.
            ws.logEvent({fail_string, actor.y, actor.x});
            return false;
        }

        // Find the target from the arguments.
        if (arguments.size() < min_arguments) {
            // TODO Should there be a generic "improper arguments" failure string?
            ws.logEvent({fail_string, actor.y, actor.x});
            return false;
        }
        return true;
    }

    // Find the target of a range 1 skill or ability, or end iterator if there is no target.
    // TODO FIXME multiple arguments are just members of an ability, just pass that here.
    // TODO FIXME Or maybe this should just be a member function of Ability?
    std::tuple<std::list<Entity>::iterator, std::tuple<size_t, size_t>> findOneTarget(WorldState& ws, Entity& actor, const std::map<std::string, nlohmann::json>& effects,
            const vector<string>& expected_args, const vector<string>& default_args, const vector<string>& args) {
        // Default to having no target.
        std::list<Entity>::iterator target = ws.entities.end();
        std::tuple<size_t, size_t> target_location{std::numeric_limits<size_t>::max(), std::numeric_limits<size_t>::max()};

        bool argument_consumed = false;
        // Find the ability range
        size_t ability_range = 0;
        if (effects.contains("range")) {
            ability_range = effects.at("range");
        }
        // Are there argument options?
        if (0 < expected_args.size() and expected_args[0] == "or") {
            // Expecting a single argument. Start with the default, but replace it with any supplied
            // argument if present.
            std::string arg = "";
            if (0 < args.size()) {
                arg = args.at(0);
            }
            else if (0 < default_args.size()) {
                arg = default_args.at(0);
            }
            // Skip the "or" when looking at possible arguments.
            auto arg_options = std::ranges::subrange(expected_args.begin()+1, expected_args.end());
            // See if we match an argument
            if (arg_options.end() != std::find(arg_options.begin(), arg_options.end(), arg)) {
                argument_consumed = true;
                // TODO FIXME What about the forward option?
                // Find the effects of this argument.
                if (effects.contains(arg)) {
                    // The arguments should control which direction to check for a target.
                    if (effects.at(arg).contains("distance")) {
                        size_t target_y = actor.y;
                        size_t target_x = actor.x;
                        auto& distances = effects.at(arg).at("distance");
                        if (distances.contains("y")) {
                            target_y += distances.at("y").get<int>();
                        }
                        if (distances.contains("x")) {
                            target_x += distances.at("x").get<int>();
                        }
                        target_location = std::make_tuple(target_y, target_x);
                        // Now find the target at that location if one exists.
                        // It is possible that there are multiple entities in that tile. This
                        // will find the first one arbitrarily.
                        target = std::find_if(ws.entities.begin(), ws.entities.end(),
                            [=](Entity& ent) { return ent.y == target_y and ent.x == target_x;});
                    }
                }
            }
        }
        // Match an arbitrary string for a <target>
        if (not argument_consumed and expected_args.end() != std::find(expected_args.begin(), expected_args.end(), ("<target>"))) {
            // Going to have to search for this target by name. Check for a passed argument, and if
            // there is none present used the default if it exists.
            std::string target_name = "";
            if (0 < args.size()) {
                target_name = args[0];
            }
            else if (0 < default_args.size()) {
                target_name = default_args[0];
            }
            // Assign the target.
            target = ws.findEntity(target_name, actor.y, actor.x, ability_range);
            // If this wasn't a name, try searching for a trait
            if (target == ws.entities.end()) {
                target = ws.findEntity(std::vector<std::string>{target_name}, actor.y, actor.x, ability_range);
            }
            if (target != ws.entities.end()) {
                target_location = std::make_tuple(target->y, target->x);
            }
        }
        // Finally return whatever iterator location was discovered.
        return {target, target_location};
    }

    // Find the target of a radius skill or ability, or end iterator if there are no targets.
    std::tuple<std::vector<std::list<Entity>::iterator>, std::set<std::tuple<size_t, size_t>>> findRadiusTarget(WorldState& ws, Entity& actor, const std::map<std::string, nlohmann::json>& effects,
            const vector<string>&, const vector<string>&, const vector<string>&) {
        // Read in the information about the radius area of effect
        const json& area_effects = effects.at("area");
        double range = area_effects.at("range").get<double>();
        // Calculate modifiers from entity attributes
        // TODO Modifiers from other attributes
        double vitality_mod = 0;
        if (area_effects.contains("vitality_mod")) {
            vitality_mod = area_effects.at("vitality_mod").get<double>();
        }
        range = floor(vitality_mod * actor.stats.value().vitality + range);

        // Radial effects don't use arguments.
        // Find all entities within the actor's range
        std::vector<std::list<Entity>::iterator> targets = ws.findEntities(std::vector<std::string>{}, actor.y, actor.x, range);
        // Fill in the area_of_effect as well.
        std::set<std::tuple<size_t, size_t>> area_of_effect;
        for (size_t step = 0; step <= range; ++step) {
            // For each distance in the range, find all combinations of y and x
            for (size_t y_dist = 0; y_dist <= step; ++y_dist) {
                size_t x_dist = step - y_dist;
                for (int ydir = -1; ydir <= 1; ydir += 2) {
                    for (int xdir = -1; xdir <= 1; xdir += 2) {
                        // Don't insert a position that would have a negative coordinate.
                        size_t target_y = actor.y + (ydir * y_dist);
                        size_t target_x = actor.x + (xdir * x_dist);
                        if ((1 == ydir or y_dist <= actor.y) and
                            (1 == xdir or x_dist <= actor.x)) {
                            area_of_effect.insert({target_y, target_x});
                        }
                    }
                }
            }
        }
        // Finally return whatever iterator locations were discovered.
        return {targets, area_of_effect};
    }

    // Find the target of a cone shaped skill or ability, or end iterator if there are no targets.
    // TODO FIXME multiple arguments are just members of an ability, just pass that here.
    // TODO FIXME Or maybe this should just be a member function of Ability?
    std::tuple<std::vector<std::list<Entity>::iterator>, std::set<std::tuple<size_t, size_t>>> findConeTarget(WorldState& ws, Entity& actor, const std::map<std::string, nlohmann::json>& effects,
            const vector<string>& expected_args, const vector<string>& default_args, const vector<string>& args) {
        // Read in the information about the cone area of effect
        const json& area_effects = effects.at("area");
        std::vector<double> range = area_effects.at("range").get<std::vector<double>>();
        double width_base = area_effects.at("width_base").get<double>();
        double width_slope = area_effects.at("width_slope").get<double>();
        // Calculate modifiers from entity attributes
        // TODO Modifiers from other attributes
        double vitality_mod = 0;
        if (area_effects.contains("vitality_mod")) {
            vitality_mod = area_effects.at("vitality_mod").get<double>();
        }
        range[1] = floor(vitality_mod * actor.stats.value().vitality + range[1]);

        // Default to having no target.
        std::vector<std::list<Entity>::iterator> targets;
        std::set<std::tuple<size_t, size_t>> area_of_effect;
        bool argument_consumed = false;
        // Are there argument options?
        if (0 < expected_args.size() and expected_args[0] == "or") {
            // Expecting a single argument. Start with the default, but replace it with any supplied
            // argument if present.
            std::string arg = "";
            if (not default_args.empty()) {
                arg = default_args.at(0);
            }
            if (0 < args.size()) {
                arg = args.at(0);
            }
            // Skip the "or" when looking at possible arguments.
            auto arg_options = std::ranges::subrange(expected_args.begin()+1, expected_args.end());
            // See if we match an argument
            if (arg_options.end() != std::find(arg_options.begin(), arg_options.end(), arg)) {
                argument_consumed = true;
                // TODO FIXME What about the forward option?
                // Find the effects of this argument.
                if (effects.contains(arg)) {
                    // The direction (in [y component, x component]) and the vector direction of the
                    // left and right sides.
                    std::vector<double> direction = effects.at(arg).at("direction").get<std::vector<double>>();
                    std::vector<double> side_direction = effects.at(arg).at("side_direction").get<std::vector<double>>();

                    for (int distance = floor(range[0]); distance <= range[1]; ++distance) {
                        int width = floor(width_base) + floor(width_slope * (distance - 1));
                        for (int lateral = -std::trunc((width-1) / 2); lateral <= std::trunc((width-1) / 2); ++lateral) {
                            // Use the directions to calculate the point being searched.
                            size_t target_y = actor.y + distance * direction[0] + lateral * side_direction[0];
                            size_t target_x = actor.x + distance * direction[1] + lateral * side_direction[1];
                            area_of_effect.insert({target_y, target_x});
                            // Now find targets at that location if they exist.
                            std::list<Entity>::iterator target = ws.entities.begin();
                            while (target != ws.entities.end()) {
                                target = std::find_if(target, ws.entities.end(),
                                    [=](Entity& ent) { return ent.y == target_y and ent.x == target_x;});
                                // If we found another match then add it to the targets.
                                if (target != ws.entities.end()) {
                                    targets.push_back(target);
                                    // Prepare for the next iteration of the loop
                                    target = std::next(target);
                                }
                            }
                        }
                    }
                }
            }
        }
        // Match an arbitrary string for a <target>
        if (not argument_consumed and expected_args.end() != std::find(expected_args.begin(), expected_args.end(), ("<target>"))) {
            // Going to have to search for this target by name. Check for a passed argument, and if
            // there is none present used the default if it exists.
            std::string target_name = "";
            if (0 < args.size()) {
                target_name = args[0];
            }
            else if (0 < default_args.size()) {
                target_name = default_args[0];
            }
            // Assign the target.
            auto target = ws.findEntity(target_name, actor.y, actor.x, range[1]);
            // If this wasn't a name, try searching for a trait
            // TODO FIXME Should this match multiples?
            if (target == ws.entities.end()) {
                target = ws.findEntity(std::vector<std::string>{target_name}, actor.y, actor.x, range[1]);
            }
            if (target != ws.entities.end()) {
                targets.push_back(target);
            }
        }
        // Finally return whatever iterator locations were discovered.
        return {targets, area_of_effect};
    }


    // A function meant for binding that increases or decreases one entity's distance from another.
    void changeDistance(Entity& actor, const Ability& ability, std::string event_string, std::string fail_string, size_t desired_distance, WorldState& ws, const std::vector<std::string>& arguments) {
        // Verify that this action could be taken.
        if (not actionBoilerplateCheck(actor, ws, ability, arguments, 1, fail_string)) {
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
        else {
            // TODO The failure comes from being unable to move. Is it necessary to have this string
            // set in the json?
            ws.logEvent({fail_string, actor.y, actor.x});
        }
    }

    std::function<void(WorldState&, const std::vector<std::string>&)> Ability::makeConditionalMoveFunction(Entity& entity) const {
        // Prepare a flavor string to go into the event log whenever this behavior occurs.
        // Fill in some fields in advance.
        std::string event_string = flavor;
        std::string fail_string = fail_flavor;
        // If this is the player then use "You" instead of the entity name.
        if (entity.traits.contains("player")) {
            replaceSubstring(event_string, "<entity>", "You");
            replaceSubstring(fail_string, "<entity>", "You");
        }
        else {
            replaceSubstring(event_string, "<entity>", entity.name);
            replaceSubstring(fail_string, "<entity>", entity.name);
        }

        if (effects.contains("minimize distance") and arguments.at(0) == "<target>") {
            return std::bind_front(changeDistance, std::ref(entity), std::cref(*this), event_string, fail_string, 0);
        }
        else if (effects.contains("maximize distance") and arguments.at(0) == "<target>") {
            return std::bind_front(changeDistance, std::ref(entity), std::cref(*this), event_string, fail_string, std::numeric_limits<size_t>::max()/2);
        }
        else if (effects.contains("maintain distance") and
                 arguments == vector<string>{"<target>", "range"}) {
            auto bound_fun = std::bind_front(changeDistance, std::ref(entity), std::cref(*this), event_string, fail_string);
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
        std::string fail_string = fail_flavor;
        if (entity.traits.contains("player")) {
            replaceSubstring(event_string, "<entity>", "You");
            replaceSubstring(fail_string, "<entity>", "You");
        }
        else {
            replaceSubstring(event_string, "<entity>", entity.name);
            replaceSubstring(fail_string, "<entity>", entity.name);
        }

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
            return [=,&entity](WorldState& ws, const vector<string>&) {
                // Ignoring the movement arguments for now.
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
            return [=,&entity,stamina=this->stamina](WorldState& ws, const vector<string>&) {
                // Ignoring the movement arguments
                // Going to use one RNG for each lambda. This theoretically protects from
                // some side channel shenanigans.
                static std::mt19937 randgen{std::random_device{}()};
                static std::uniform_int_distribution<> rand_direction(0, 1);
                static std::uniform_int_distribution<> rand_distance(rand_min, rand_max);
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

    void informationFunction(Entity& actor, const Ability& ability, std::vector<std::string> info_types, std::string event_string, std::string fail_string, WorldState& ws, const std::vector<std::string>& arguments) {
        // Verify that this action can be taken.
        // TODO FIXME Should have a better way to find the minimum arguments
        size_t min_arguments = 1;
        if (ability.arguments.empty()) {
            min_arguments = 0;
        }
        if (not actionBoilerplateCheck(actor, ws, ability, arguments, min_arguments, fail_string)) {
            return;
        }

        // Keep track of detected entities and update them in the information section of the status
        // window.

        std::vector<std::list<Entity>::iterator> targets;
        std::set<std::tuple<size_t, size_t>> area_of_effect;
        if (AbilityArea::single == ability.area) {
            auto [target, target_location] = findOneTarget(ws, actor, ability.effects, ability.arguments, ability.default_args, arguments);

            if (ws.entities.end() != target) {
                targets.push_back(target);
            }
        }
        else if (AbilityArea::cone == ability.area) {
            std::tie(targets, area_of_effect) = findConeTarget(ws, actor, ability.effects, ability.arguments, ability.default_args, arguments);
        }
        else if (AbilityArea::radius == ability.area) {
            std::tie(targets, area_of_effect) = findRadiusTarget(ws, actor, ability.effects, ability.arguments, ability.default_args, arguments);
        }

        // Mark background colors for the area of effect
        for (auto& location : area_of_effect) {
            if (std::get<0>(location) < ws.field_height and std::get<1>(location) < ws.field_width) {
                // TODO Hard-coding the utility color to be cyan here.
                ws.background_effects.insert({location, "cyan"});
            }
        }

        // Handle targets if there are any.
        if (not targets.empty()) {
            // Subtract the ability's stamina cost.
            actor.stats.value().stamina -= ability.stamina;
            // Handle each observed target.
            for (auto target : targets) {
                std::string target_event_string = event_string;
                replaceSubstring(target_event_string, "<target>", target->name);
                // Log the success event
                ws.logEvent({target_event_string, actor.y, actor.x});
                // Now log any observable information.
                // Pull out traits that are observable with the given information_types.
                std::vector<std::wstring> target_information;
                target_information.push_back(std::wstring(target->name.begin(), target->name.end()));
                for (const std::string& info_type : info_types) {
                    if (target->description.contains(info_type)) {
                        target_information.push_back(target->description.at(info_type));
                    }
                }
                // Send this information to the status window
                if (not target_information.empty()) {
                    ws.logInformation(target_information);
                }
            }
        }
        else {
            // Otherwise log the failure string
            ws.logEvent({fail_string, actor.y, actor.x});
        }
    }

    void equipFunction(Entity& actor, const Ability& ability, const std::string& equip_type, std::string event_string, std::string fail_string, WorldState& ws, const std::vector<std::string>& arguments) {
        // Verify that this action can be taken.
        // TODO FIXME Should have a better way to find the minimum arguments
        size_t min_arguments = 1;
        if (ability.arguments.empty()) {
            min_arguments = 0;
        }
        if (not actionBoilerplateCheck(actor, ws, ability, arguments, min_arguments, fail_string)) {
            return;
        }

        // Keep track of detected entities and update them in the information section of the status
        // window.

        std::vector<std::list<Entity>::iterator> targets;
        std::set<std::tuple<size_t, size_t>> area_of_effect;
        // TODO The search functions should also search the world states contained by inventory on the user.
        if (AbilityArea::single == ability.area) {
            auto [target, target_location] = findOneTarget(ws, actor, ability.effects, ability.arguments, ability.default_args, arguments);

            if (ws.entities.end() != target) {
                targets.push_back(target);
            }
        }
        else if (AbilityArea::cone == ability.area) {
            std::tie(targets, area_of_effect) = findConeTarget(ws, actor, ability.effects, ability.arguments, ability.default_args, arguments);
        }
        else if (AbilityArea::radius == ability.area) {
            std::tie(targets, area_of_effect) = findRadiusTarget(ws, actor, ability.effects, ability.arguments, ability.default_args, arguments);
        }

        // Mark background colors for the area of effect
        for (auto& location : area_of_effect) {
            if (std::get<0>(location) < ws.field_height and std::get<1>(location) < ws.field_width) {
                // TODO Hard-coding the utility color to be cyan here.
                ws.background_effects.insert({location, "cyan"});
            }
        }

        // Handle targets if there are any.
        if (not targets.empty()) {
            // Subtract the ability's stamina cost and attempt to equip items.
            actor.stats.value().stamina -= ability.stamina;
            // Assign each item to a slot
            for (std::list<Entity>::iterator equipment : targets) {
                // This ability affects a slot, right? Was it provided, or will it be inferred?
                std::string target_slot = "";
                if (2 <= arguments.size() and 2 <= ability.arguments.size() and ability.arguments.at(1) == "<slot>") {
                    target_slot = arguments.at(1);
                }
                else {
                    // Find a slot to equip this item.
                    // Check the entity.possible_slots to see where it could fit, and check
                    // entity.occupid_slots to see if there is a free slot.
                    auto possible_slot = actor.possible_slots.end();
                    auto next_slot = actor.possible_slots.begin();
                    while (next_slot != actor.possible_slots.end()) {
                        next_slot = std::find_if(next_slot, actor.possible_slots.end(),
                                [&](const std::string& slot) {return actor.canEquip(*equipment, slot);});
                        // If a possible slot was found check its status
                        if (actor.possible_slots.end() != next_slot) {
                            // If this isn't occupied then a final slot has been found. Otherwise
                            // set this as a possible slot, but don't top looping yet.
                            possible_slot = next_slot;
                            if (actor.occupied_slots.contains(*next_slot)) {
                                // Check the rest of the slots
                                std::advance(next_slot, 1);
                            }
                            else {
                                // Done checking
                                next_slot = actor.possible_slots.end();
                            }
                        }
                    }

                    // Check if this item can be equipped
                    if (actor.possible_slots.end() != possible_slot) {
                        // Create the message for this action
                        std::string target_event_string = event_string;
                        replaceSubstring(target_event_string, "<target>", equipment->name);
                        replaceSubstring(target_event_string, "<slot>", *possible_slot);
                        // Reset equipment's current location
                        equipment->y = 0;
                        equipment->x = 0;
                        // Equip and remove from the world state
                        std::optional<Entity> swapped = actor.equip(*equipment, *possible_slot);
                        ws.entities.erase(equipment);
                        // If we swapped equipment then this should be dropped into the same
                        // location as the actor
                        if (swapped) {
                            // Put swapped equipment into actor's location
                            swapped.value().y = actor.y;
                            swapped.value().x = actor.x;
                            std::string drop_string = actor.name + " drops " + swapped.value().name + ".";
                            ws.logEvent({drop_string, actor.y, actor.x});
                            ws.entities.push_back(std::move(swapped.value()));
                        }
                        // Log the equip event.
                        ws.logEvent({target_event_string, actor.y, actor.x});
                    }
                    else {
                        // Otherwise log the failure string
                        std::string target_fail_string = fail_string;
                        replaceSubstring(target_fail_string, "<target>", equipment->name);
                        ws.logEvent({target_fail_string, actor.y, actor.x});
                    }
                }
            }
        }
        else {
            // Otherwise log the failure string
            std::string target_fail_string = fail_string;
            replaceSubstring(target_fail_string, "<target>", "something");
            ws.logEvent({target_fail_string, actor.y, actor.x});
        }
    }

    std::function<void(WorldState&, const std::vector<std::string>&)> Ability::makeUtilityFunction(Entity& entity) const {
        // Prepare a flavor string to go into the event log whenever this behavior occurs.
        // Fill in some fields in advance.
        std::string event_string = flavor;
        std::string fail_string = fail_flavor;
        // If this is the player then use "You" instead of the entity name.
        if (entity.traits.contains("player")) {
            replaceSubstring(event_string, "<entity>", "You");
            replaceSubstring(fail_string, "<entity>", "You");
        }
        else {
            replaceSubstring(event_string, "<entity>", entity.name);
            replaceSubstring(fail_string, "<entity>", entity.name);
        }


        // See if this is an information skill
        if (effects.contains("information")) {
            std::vector<std::string> information_types = effects.at("information");

            // Information utility function.
            return std::bind_front(informationFunction, std::ref(entity), std::cref(*this), information_types, event_string, fail_string);

        }
        else if (effects.contains("equip")) {
            std::string equip_type = effects.at("equip");

            // Information utility function.
            return std::bind_front(equipFunction, std::ref(entity), std::cref(*this), equip_type, event_string, fail_string);
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
        double reflexes = 0;
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
            if (damage_effects.contains("reflexes")) {
                reflexes = damage_effects.at("reflexes");
            }
        }
        // TODO Or maybe just copy over any effects that are also in the expected arguments?
        std::vector<std::string> expected_args = arguments;
        std::vector<std::string> default_args = this->default_args;
        // TODO Make different classes for range and area combinations
        // Prepare a flavor string to go into the event log whenever this behavior occurs.
        // Fill in some fields in advance.
        std::string event_string = flavor;
        std::string fail_string = fail_flavor;
        // If this is the player then use "You" instead of the entity name.
        if (entity.traits.contains("player")) {
            replaceSubstring(event_string, "<entity>", "You");
            replaceSubstring(fail_string, "<entity>", "You");
        }
        else {
            replaceSubstring(event_string, "<entity>", entity.name);
            replaceSubstring(fail_string, "<entity>", entity.name);
        }

        return [=,&entity,effects=this->effects,stamina=this->stamina](WorldState& ws, const vector<string>& args) {
            size_t damage = floor(base + strength * entity.stats.value().strength + domain * entity.stats.value().domain +
                aura * entity.stats.value().aura + reflexes * entity.stats.value().reflexes);
            // Now parse the arguments to see what is getting hit.
            auto [target, target_location] = findOneTarget(ws, entity, effects, expected_args, default_args, args);
            if (target != ws.entities.end()) {
                std::string log_string = event_string;
                replaceSubstring(log_string, "<target>", target->name);
                ws.logEvent({log_string, target->y, target->x});

                // Deal damage to the target
                ws.damageEntity(target, damage, entity);
            }
            else {
                ws.logEvent({fail_string, entity.y, entity.x});
            }
            // Visually mark the tile if it is on the map
            if (std::get<0>(target_location) < ws.field_height and std::get<1>(target_location) < ws.field_width) {
                // TODO Hard-coding the attack color to be red here.
                ws.background_effects.insert({target_location, "red"});
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
        else if (type == AbilityType::utility) {
            return makeUtilityFunction(entity);
        }
        // Otherwise return a nothing
        return noop_function;
    }

    // Find the ability names that are available to this entity within this behavior set.
    std::vector<std::string> AbilitySet::getAvailable(const Entity& entity) const {
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
    std::vector<std::string> AbilitySet::updateAvailable(Entity& entity) const {
        std::vector<std::string> available;

        // Check which abilities this entity should be able to use.
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

                // Automatically alias "attack" to the strongest single stamina attack available.
                if (AbilityType::attack == ability.type) {
                    // TODO The strongest attack type
                    available.push_back("attack");
                    entity.command_handlers.insert({"attack", entity.command_handlers.at(ability_name)});
                    entity.command_details.insert({"attack", entity.command_details.at(ability_name)});
                }
            }
        }

        return available;
    }


    std::function<void(WorldState&, const std::vector<std::string>&)> AbilitySet::makeFunction(const std::string& ability, Entity& entity) const {
        // Make the function for this ability.
        if (not abilities.contains(ability)) {
            return noop_function;
        }
        return abilities.at(ability).makeFunction(entity);
    }

    const std::vector<AbilitySet>& getAbilities() {
        if (0 < loaded_abilities.size()) {
            return loaded_abilities;
        }
        // Otherwise we need to load the json file and populate the behaviors.
        json abilities = loadJson("resources/behavior.json");
        // Go through the json and translate all of the entries into new AbilitySets.
        for (auto& [ability_name, ability_json] : abilities.get<std::map<std::string, json>>()) {
            loaded_abilities.push_back(AbilitySet(ability_name, ability_json));
        }
        return loaded_abilities;
    }

    const std::map<std::string, BehaviorSet>& getBehaviors() {
        if (0 < loaded_behaviors.size()) {
            return loaded_behaviors;
        }
        // Otherwise we need to load the json file and populate the behaviors.
        json behaviors = loadJson("resources/behavior_set.json");
        // Go through the json and translate all of the entries into new BehaviorSets.
        for (auto& [behavior_name, behavior_json] : behaviors.get<std::map<std::string, json>>()) {
            std::string description = behavior_json.at("description").get<std::string>();
            std::vector<std::vector<std::string>> rules;
            behavior_json.at("rules").get_to(rules);
            loaded_behaviors.insert({behavior_name, {behavior_name, description, rules}});
        }
        return loaded_behaviors;
    }

    std::function<bool(double, double)> strToCompFn(const std::string& str) {
        if ("<" == str) {
            return std::less<double>{};
        }
        else if (">" == str) {
            return std::greater<double>{};
        }
        else if ("<=" == str) {
            return std::less_equal<double>{};
        }
        else if (">=" == str) {
            return std::greater_equal<double>{};
        }
        else if ("!=" == str) {
            return std::not_equal_to<double>{};
        }
        else if ("==" == str) {
            return std::equal_to<double>{};
        }
        // Return an empty function if there was no match.
        return [](double, double) { return false; };
    }

    void BehaviorSet::executeBehavior(Entity& entity, WorldState& ws, CommandHandler& comham) const{
        // Go through the behavior set of the given entity and follow its rules to take appropriate
        // actions.
        const std::regex hp_condition("hp ([<>]) ([0-9]+)%");
        const std::regex distance_condition("distance:([a-z]+) ([<>]) ([0-9]+)");
        const std::regex detect_condition("sense ([a-z]+)");
        const std::regex else_condition("else");

        std::smatch matches;
        // Check entity.behavior_set_name to ensure that the entity has a valid behavior pattern.
        if (loaded_behaviors.contains(entity.behavior_set_name)) {
            // Need to remember if any actions were taken when we reach any "else" rule conditions.
            bool any_action_taken = false;

            // TODO FIXME These could be preprocessed once when the BehaviorSet is constructed
            // instead of being reprocessed every time the behavior is executed.
            const BehaviorSet& bset = loaded_behaviors.at(entity.behavior_set_name);
            for (const std::vector<std::string>& rule_actions : bset.rules) {
                bool do_actions = false;
                const std::string& rule = rule_actions.at(0);
                // Check if this rule is a hit point condition
                if (std::regex_match(rule, matches, hp_condition)) {
                    std::string comparison = matches[1].str();
                    // Read the threshold and convert from percent.
                    double threshold = stod(matches[2].str())/100.0;
                    if (entity.stats) {
                        Stats& stats = entity.stats.value();
                        double hp_percent = (double)stats.health / stats.maxHealth();
                        auto comp_fn = strToCompFn(comparison);
                        // Take the actions if the comparison is true.
                        do_actions = comp_fn(hp_percent, threshold);
                    }
                }
                // Check if this rule is a distance condition
                else if (std::regex_match(rule, matches, distance_condition)) {
                    // Unpack the values from the match
                    std::string target = matches[1].str();
                    auto comp_fn = strToCompFn(matches[2].str());
                    size_t range = stoull(matches[3].str());
                    // An entity cannot sense anything beyond its detection range.
                    if (entity.stats) {
                        range = std::min(range, entity.stats.value().detectionRange());
                    }
                    // Assume that the target is a trait and search for it.
                    auto target_entity_i = ws.findEntity(std::vector<std::string>{target}, entity.y, entity.x, range);
                    // If that didn't match, then search via a name
                    if (ws.entities.end() == target_entity_i) {
                        target_entity_i = ws.findEntity(target, entity.y, entity.x, range);
                    }

                    // Now check the distance threshold if the target entity was found.
                    if (ws.entities.end() != target_entity_i) {
                        size_t distance = std::abs((int)entity.y - (int)target_entity_i->y) +
                                          std::abs((int)entity.x - (int)target_entity_i->x);
                        do_actions = comp_fn(distance, range);
                    }
                }
                // Check if this rule is a detection condition
                if (std::regex_match(rule, matches, detect_condition)) {
                    // Unpack the values from the match
                    std::string target = matches[1].str();
                    // Check if this entity has a detection range.
                    if (entity.stats) {
                        size_t range =  entity.stats.value().detectionRange();
                        // Assume that the target is a trait and search for it.
                        auto target_entity_i = ws.findEntity(std::vector<std::string>{target}, entity.y, entity.x, range);
                        // If that didn't match, then search via a name
                        if (ws.entities.end() == target_entity_i) {
                            target_entity_i = ws.findEntity(target, entity.y, entity.x, range);
                        }
                        // If the entity was detected then do the actions.
                        do_actions = ws.entities.end() != target_entity_i;
                    }
                }
                // Check if this rule is an else condition.
                if (std::regex_match(rule, matches, else_condition)) {
                    // Take the else actions if no other actions were taken.
                    do_actions = not any_action_taken;
                }
                any_action_taken = any_action_taken or do_actions;
                if (do_actions) {
                    // Put the actions into the command handler.
                    // Actions are everything in rule_actions from index 1 onward.
                    for (size_t idx = 1; idx < rule_actions.size(); ++idx) {
                        //TODO it would be nice if we got world state changes in between
                        //actions.
                        //Easy enough to craft the AI rules around this limitation though.
                        comham.enqueueEntityCommand(entity, rule_actions.at(idx));
                    }
                }
            }
        }
    }
}
