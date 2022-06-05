/*
 * Copyright 2022 Bernhard Firner
 *
 * A command handler for entities in the game.
 * Command may or may not come from the player.
 * It is easy enough to think of this as the game's logic unit.
 */

#include <algorithm>
#include <cctype>
#include <functional>
#include <unordered_map>
#include <set>
#include <string>
#include <vector>

using std::string;
using std::vector;

#include "command_handler.hpp"
#include "entity.hpp"
#include "world_state.hpp"

// Where we can find all of the handlers
std::unordered_map<std::string, std::function<void(Entity&, WorldState&, const vector<string>&)>> command_handlers{
};

// Go through all of the command handlers, take the keys, and insert any non-overlapping
// abbreviations.
void createAbbreviations(decltype(command_handlers)& handlers) {
    std::vector<string> keys;
    for (const auto& [key, value] : handlers) {
        keys.push_back(key);
    }

    // Resolve all keys first before trying to resolve any name collisions.
    std::set<string> collisions;
    for (const std::string& key : keys) {
        // Insert entries for everything from key.substr(0, 1) to key.substr(0, key.size()-1)
        // If there is ever a collision then add this to the collision list and remove them at the
        // end.
        for (size_t sublen = 1; sublen < key.size(); ++sublen) {
            string substr = key.substr(0, sublen);
            if (handlers.contains(substr)) {
                collisions.insert(substr);
            }
            else {
                command_handlers.insert(std::make_pair(substr, command_handlers[key]));
            }
        }
    }

    std::erase_if(command_handlers, [&](const auto& item) {
        auto const& [key, value] = item;
        return collisions.contains(key);
    });
}

CommandHandler::CommandHandler() {
    createAbbreviations(command_handlers);
}

/*
 * Split the repitition part of the string from the @command. The returned value is the number of
 * times to repeat the command string.
 */
size_t parseRepititions(string& command) {
    std::string::size_type split_pos = command.find(' ') ;
    if (string::npos != split_pos) {
        string first_part = command.substr(0, split_pos);
        if (std::all_of(first_part.begin(), first_part.end(), isdigit)) {
            size_t reps = std::stoul(first_part);
            command = command.substr(split_pos+1);
            return reps;
        }
    }
    // Otherwise the command doesn't have a numeric modifier so perform it once.
    return 1;
}

std::vector<string> parseArguments(string& command) {
    std::string::size_type space_idx = command.find(' ');
    std::vector<string> arguments;
    if (std::string::npos != space_idx) {
        string command_part = command.substr(0, space_idx);
        while (std::string::npos != space_idx) {
            std::string::size_type next_idx = command.find(' ', space_idx+1);
            std::string next_argument = command.substr(space_idx+1, next_idx-space_idx-1);
            if (" " != next_argument) {
                arguments.push_back(next_argument);
            }
            space_idx = next_idx;
        }
        command = command_part;
    }
    return arguments;
}

// A command for a specific entity
void CommandHandler::enqueueNamedEntityCommand(const std::string& entity, const std::string& command) {
    string new_command = command;
    size_t reps = parseRepititions(new_command);

    // Now split off the arguments
    std::vector<string> arguments = parseArguments(new_command);
    for (size_t i = 0; i < reps; ++i) {
        named_entity_commands.push_back({entity, new_command, arguments});
    }
}

// A command for all entities with the given trait
void CommandHandler::enqueueTraitCommand(const std::vector<std::string>& traits, const std::string& command) {
    string new_command = command;
    size_t reps = parseRepititions(new_command);

    // Now split off the arguments
    std::vector<string> arguments = parseArguments(new_command);
    for (size_t i = 0; i < reps; ++i) {
        trait_commands.push_back({traits, new_command, arguments});
    }
}

void CommandHandler::enqueueEntityRefCommand(decltype(WorldState::entities)::iterator entity_i, const std::string& command) {
    string new_command = command;
    size_t reps = parseRepititions(new_command);

    // Now split off the arguments
    std::vector<string> arguments = parseArguments(new_command);
    for (size_t i = 0; i < reps; ++i) {
        // It's too dangerous to store iterators to a container that this class has no control over,
        // so store the entity IDs instead.
        entity_commands.push_back({entity_i->entity_id, new_command, arguments});
    }
}

void CommandHandler::enqueueEntityCommand(const Entity& entity, const std::string& command) {
    string new_command = command;
    size_t reps = parseRepititions(new_command);

    // Now split off the arguments
    std::vector<string> arguments = parseArguments(new_command);
    for (size_t i = 0; i < reps; ++i) {
        entity_commands.push_back({entity.entity_id, new_command, arguments});
    }
}

// Execute all enqueued commands. Entity commands will always occur before trait commands.
void CommandHandler::executeCommands(WorldState& ws) {
    // First handle commands to entity names and traits by putting them into the regular
    // entity_commands queue. Sort the queue by reflex speed, and then take all actions.

    // Handle all {name, command} pairs if they both exist
    for (const auto& [entity_name, command, arguments] : named_entity_commands) {
        auto entity_i = ws.findEntity(entity_name);
        if (entity_i != ws.entities.end() and entity_i->command_handlers.contains(command)) {
            //entity_i->command_handlers.at(command)(ws, arguments);
            entity_commands.push_back({entity_i->entity_id, command, arguments});
        }
    }
    named_entity_commands.clear();

    // Handle all {traits, command} pairs if we can find entities with matching traits.
    for (const auto& [entity_traits, command, arguments] : trait_commands) {
        // Find any entities with all matching traits
        for (Entity& entity : ws.entities) {
            if (std::all_of(entity_traits.begin(), entity_traits.end(),
                [&](const std::string& trait) { return entity.traits.contains(trait);})) {
                // This entity has all of the necessary traits, so execute the command if it is
                // supported.
                if (entity.command_handlers.contains(command)) {
                    //entity.command_handlers.at(command)(ws, arguments);
                    entity_commands.push_back({entity.entity_id, command, arguments});
                }
            }
        }
    }
    trait_commands.clear();

    // TODO Sort commands by reflexes
    // TODO Sort commands by reflexes, but successive commands by the same entity happen later in
    // the round.

    // Handle all {entity iterator, command, arguments}
    for (const auto& [entity_id, command, arguments] : entity_commands) {
        auto entity_i = ws.findEntity(entity_id);
        if (entity_i != ws.entities.end() and entity_i->command_handlers.contains(command)) {
            entity_i->command_handlers.at(command)(ws, arguments);
        }
    }
    entity_commands.clear();
}
