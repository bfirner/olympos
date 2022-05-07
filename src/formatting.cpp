/*
 * Copyright 2022 Bernhard Firner
 *
 * Handles display formatting of different objects in the game.
 */

#include <cctype>

#include "formatting.hpp"

char getEntityChar(const Entity& ent) {
    if (ent.traits.contains("player")) {
        return '@';
    }
    else if (ent.traits.contains("wall")) {
        return '#';
    }
    else if (ent.traits.contains("aggro")) {
        char c = 'M';
        if (ent.traits.contains("flying")) {
            c = 'W';
        }

        if (ent.traits.contains("small")) {
            c = tolower(c);
        }
        return c;
    }
    // Default is an empty space.
    return ' ';
}

