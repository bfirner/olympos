/*
 * Copyright 2022 Bernhard Firner
 *
 * Entity in the game. Has a position, a name, and some traits.
 */

#pragma once

#include <set>
#include <string>

struct Entity {
    size_t y = 0;
    size_t x = 0;
    std::string name;

    std::set<std::string> traits;
};
