/*
 * Copyright 2022 Bernhard Firner
 *
 * A command handler for entities in the game.
 * Command may or may not come from the player.
 * This queue is used to ensure an orderly execution of commands.
 */

#pragma once

#include <string>
#include <tuple>
#include <vector>

class CommandHandler;

#include "entity.hpp"
#include "world_state.hpp"

class CommandHandler {
    private:
        // TODO FIXME These could all look up the entity iterators and then there would be only one
        // queue instead of three.
        std::vector<std::tuple<std::string, std::string, std::vector<std::string>>> named_entity_commands;
        std::vector<std::tuple<std::vector<std::string>, std::string, std::vector<std::string>>> trait_commands;
        std::vector<std::tuple<std::list<Entity>::iterator, std::string, std::vector<std::string>>> entity_ref_commands;
        std::vector<std::tuple<size_t, std::string, std::vector<std::string>>> entity_commands;

    public:
        CommandHandler();

        // TODO FIXME Add argument lists for all of the commands
        // TODO FIXME Events, such as recovery, that occur every tick.

        // A command for an entity by its name
        void enqueueNamedEntityCommand(const std::string& entity, const std::string& command);

        // A command for a referenced entity
        void enqueueEntityRefCommand(std::list<Entity>::iterator entity_i, const std::string& command);

        // A command for all entities with the given trait
        void enqueueTraitCommand(const std::vector<std::string>& traits, const std::string& command);

        // A command for the given entity.
        void enqueueEntityCommand(const Entity& entity, const std::string& command);

        // Execute all enqueued commands. Entity commands will always occur before trait commands.
        void executeCommands(WorldState& ws);
};
