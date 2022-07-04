/*
 * Copyright 2022 Bernhard Firner
 *
 * Inventory of a container.
 */

#include <stdexcept>

#include "inventory.hpp"

// Constructor
Inventory::Inventory(const std::string& name, size_t capacity, std::set<std::string> restricted_traits) : capacity(capacity), restricted_traits(restricted_traits), name(name) {
}

// Attempt to insert an item into inventory. Returns success.
// Upon successful insertion the original entity is no longer a value object.
bool Inventory::insert(Entity& entity) {
    // Cannot insert if we are over capacity.
    if (entities.size() >= capacity) {
        return false;
    }
    // Cannot insert if this item is restricted from this container.
    for (const std::string& trait : restricted_traits) {
        if (entity.traits.contains(trait)) {
            return false;
        }
    }
    entities.push_back(std::move(entity));
    return true;
}

// True if the container contains the specified object
bool Inventory::contains(const std::string& name_or_trait) {
    return entities.end() != std::find_if(entities.begin(), entities.end(),
            [&](Entity& ent) {return ent.name == name_or_trait or ent.traits.contains(name_or_trait);});
}

// The contents of the container as a list of strings.
std::vector<std::string> Inventory::contents() {
    std::vector<std::string> names;
    for (const Entity& ent : entities) {
        names.push_back(ent.name);
    }
    return names;
}

// Remove an item from inventory.
// May throw an exception if this entity does not exist.
Entity Inventory::remove(const std::string& name_or_trait) {
    std::list<Entity>::iterator found = std::find_if(entities.begin(), entities.end(),
            [&](Entity& ent) {return ent.name == name_or_trait or ent.traits.contains(name_or_trait);});
    if (entities.end() == found) {
        throw std::runtime_error("Attempt to remove inventory item that does not exist.");
    }
    Entity ent(std::move(*found));
    entities.erase(found);
    return ent;
}

Entity Inventory::remove(size_t idx) {
    if (entities.size() >= idx) {
        throw std::runtime_error("Attempt to remove inventory item that does not exist.");
    }
    std::list<Entity>::iterator found = entities.begin();
    std::advance(found, idx);
    Entity ent(std::move(*found));
    entities.erase(found);
    return ent;
}
