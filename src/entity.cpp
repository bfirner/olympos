/*
 * Copyright 2022 Bernhard Firner
 *
 * Entity in the game. Has a position, a name, and some traits.
 */

#include <algorithm>
#include <set>
#include <string>

#include "entity.hpp"

std::string Entity::getSpecies() const {
    auto species_location = std::find_if(traits.begin(), traits.end(),
            [](const std::string& entry){ return entry.starts_with("species:");});
    if (species_location == traits.end()) {
        return "";
    }
    return species_location->substr(std::string("species:").size());
}
