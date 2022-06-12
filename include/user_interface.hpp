/*
 * Copyright 2022 Bernhard Firner
 *
 * Handles display formatting of different objects in the game.
 */

#pragma once

#include <nlohmann/json.hpp>

#include <ncurses.h>

#include <deque>
#include <list>
#include <string>
#include <tuple>
#include <vector>

#include "entity.hpp"

using json = nlohmann::json;

namespace UserInterface {
    // Get the character representation for this entity
    std::wstring getEntityChar(const Entity& ent);
    attr_t getEntityAttr(const Entity& ent);
    short getEntityColor(const Entity& ent, const std::string& bg_color = "black");


    // Update all of the entities onto the given window. Also color the backgrounds of tiles to
    // indicate effect areas.
    void updateDisplay(WINDOW* window, const std::list<Entity>& entities, const std::map<std::tuple<size_t, size_t>, std::string>& background_effects = {});
    // Clear the user input area
    void clearInput(WINDOW* window, size_t field_height, size_t field_width);
    // Setup colors
    void setupColors();

    void drawString(WINDOW* window, const std::string& str, size_t row, size_t column);
    void drawString(WINDOW* window, const std::wstring& str, size_t row, size_t column);

    // TODO FIXME The status, infolog, and hotkeys are all status window specific. UIComponent could
    // be specialized to support a status window class instead of these window specific items living
    // here.
    // Update the status and return the last row used.
    size_t drawStatus(WINDOW* window, const Entity& entity, size_t row, size_t column);

    size_t drawInfolog(WINDOW* window, size_t row, std::deque<std::vector<std::wstring>> info_log);

    // Draw hotkey shortcuts and return the last row used.
    size_t drawHotkeys(WINDOW* window, size_t row, const std::vector<std::string>& shortcuts);

    void updateEvents(WINDOW* window, std::deque<std::string>& buffer);

    // Check if there is dialogue available for the given string.
    bool hasDialogue(const std::string& dialogue_name);

    // Fetch available dialogue for the given string. This should not be called unless hasDialogue
    // returns true for the given dialogue_name.
    json& getDialogue(const std::string& dialogue_name);
}
