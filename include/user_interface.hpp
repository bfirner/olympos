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
    std::wstring getEntityChar(const Entity& ent);
    attr_t getEntityAttr(const Entity& ent);
    short getEntityColor(const Entity& ent);


    // Update all of the entities onto the given window
    void updateDisplay(WINDOW* window, const std::list<Entity>& entities);
    // Clear the user input area
    void clearInput(WINDOW* window, size_t field_height, size_t field_width);
    // Setup colors
    void setupColors();

    void drawString(WINDOW* window, const std::string& str, size_t row, size_t column);
    void drawString(WINDOW* window, const std::wstring& str, size_t row, size_t column);
    // Update the status and return the last row used.
    size_t drawStatus(WINDOW* window, const Entity& entity, size_t row, size_t column);

    void updateEvents(WINDOW* window, std::deque<std::string>& buffer, size_t line_size);

    // Draw hotkey shortcuts and return the last row used.
    size_t drawHotkeys(WINDOW* window, size_t row, const std::vector<std::string>& shortcuts);
    void drawPause(WINDOW* window, size_t rows, size_t columns);

    bool hasDialogue(const std::string& dialogue_name);
    void renderDialogue(WINDOW* window, const std::string& dialogue_name, size_t rows, size_t columns);
}
