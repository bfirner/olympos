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
#include <vector>
#include <set>
#include <string>

using std::string;

#include "command_handler.hpp"
#include "entity.hpp"
#include "world_state.hpp"

/*
 * All known commands and how to handle them.
 */

void handleEast(Entity& entity, WorldState& ws) {
    if (ws.passable[entity.y][entity.x+1]) {
        entity.x += 1;
    }
}

void handleNorth(Entity& entity, WorldState& ws) {
    if (ws.passable[entity.y-1][entity.x]) {
        entity.y -= 1;
    }
}

void handleSouth(Entity& entity, WorldState& ws) {
    if (ws.passable[entity.y+1][entity.x]) {
        entity.y += 1;
    }
}

void handleWest(Entity& entity, WorldState& ws) {
    if (ws.passable[entity.y][entity.x-1]) {
        entity.x -= 1;
    }
}

// Where we can find all of the handlers
std::unordered_map<std::string, std::function<void(Entity&, WorldState&)>> command_handlers{
    {"east", handleEast},
    {"north", handleNorth},
    {"south", handleSouth},
    {"west", handleWest},
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

// A command for a specific entity
void CommandHandler::enqueueEntityCommand(const std::string& entity, const std::string& command) {
    string new_command = command;
    size_t reps = parseRepititions(new_command);
    for (size_t i = 0; i < reps; ++i) {
        entity_commands.push_back({entity, new_command});
    }
}

// A command for all entities with the given trait
void CommandHandler::enqueueTraitCommand(const std::vector<std::string>& traits, const std::string& command) {
    string new_command = command;
    size_t reps = parseRepititions(new_command);
    for (size_t i = 0; i < reps; ++i) {
        trait_commands.push_back({traits, new_command});
    }
}

// Execute all enqueued commands. Entity commands will always occur before trait commands.
void CommandHandler::executeCommands(WorldState& ws) {
    // Handle all name, command pairs if they both exist
    for (const auto& [entity_name, command] : entity_commands) {
        if (ws.named_entities.contains(entity_name) and
                command_handlers.contains(command)) {
            command_handlers[command](*ws.named_entities[entity_name], ws);
        }
    }
    entity_commands.clear();

    // Handle all traits, command pairs if we can find entities with matching traits.
    for (const auto& [entity_traits, command] : trait_commands) {
        if (command_handlers.contains(command)) {
            // Find any entities with all matching traits
            for (Entity& entity : ws.entities) {
                if (std::all_of(entity_traits.begin(), entity_traits.end(),
                    [&](const std::string& trait) { return entity.traits.contains(trait);})) {
                    command_handlers[command](entity, ws);
                }
            }
        }
    }
    trait_commands.clear();
}
