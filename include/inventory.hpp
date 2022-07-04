/*
 * Copyright 2022 Bernhard Firner
 *
 * Inventory of a container.
 */

#pragma once

#include <list>
#include <set>
#include <string>
#include <vector>

#include "entity.hpp"

class Inventory {
    private:
        // The items inside of this container
        std::list<Entity> entities;

        // The number of items that this container can hold.
        size_t capacity;

        // Types of things that cannot be stored in this container.
        std::set<std::string> restricted_traits;
    public:

        // Name of this container
        std::string name;

        // Constructor
        Inventory(const std::string& name, size_t capacity, std::set<std::string> restricted_traits);

        // Attempt to insert an item into inventory. Returns success.
        // Upon successful insertion the original entity is no longer a value object.
        bool insert(Entity& entity);

        // True if the container contains the specified object
        bool contains(const std::string& name_or_trait);

        // The contents of the container as a list of strings.
        std::vector<std::string> contents();

        // Remove an item from inventory.
        // May throw an exception if this entity does not exist.
        Entity remove(const std::string& name_or_trait);
        Entity remove(size_t idx);
};
