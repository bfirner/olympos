/*
 * Copyright 2022 Bernhard Firner
 *
 * Handles display formatting of different objects in the game.
 */

#pragma once

#include <ncurses.h>

#include <deque>
#include <list>
#include <string>
#include <vector>

#include "entity.hpp"

namespace UserInterface {
    // Get the character representation for this entity
    char getEntityChar(const Entity& ent);
    attr_t getEntityAttr(const Entity& ent);
    short getEntityColor(const Entity& ent);


    // Update all of the entities onto the given window
    void updateDisplay(WINDOW* window, const std::list<Entity>& entities);
    // Clear the user input area
    void clearInput(WINDOW* window, size_t field_height, size_t field_width);
    // Setup colors
    void setupColors();

    void drawString(WINDOW* window, const std::string& str, size_t row, size_t column);
    void drawStatus(WINDOW* window, const Entity& entity, size_t row, size_t column);

    void updateEvents(WINDOW* window, std::deque<std::string>& buffer);
}
