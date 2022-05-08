/*
 * Copyright 2022 Bernhard Firner
 *
 * A command handler for entities in the game.
 * Command may or may not come from the player.
 * It is easy enough to think of this as the game's logic unit.
 */

#pragma once

#include <string>
#include <vector>

#include "entity.hpp"
#include "world_state.hpp"

class CommandHandler {
    private:
        std::vector<std::pair<std::string, std::string>> entity_commands;
        std::vector<std::pair<std::vector<std::string>, std::string>> trait_commands;

    public:
        CommandHandler();

        // A command for a specific entity
        void enqueueEntityCommand(const std::string& entity, const std::string& command);

        // A command for all entities with the given trait
        void enqueueTraitCommand(const std::vector<std::string>& traits, const std::string& command);

        // Execute all enqueued commands. Entity commands will always occur before trait commands.
        void executeCommands(WorldState& ws);
};
