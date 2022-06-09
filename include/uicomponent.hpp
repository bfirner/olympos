/*
 * Copyright 2022 Bernhard Firner
 *
 * A UI component.
 */

#pragma once

#include <nlohmann/json.hpp>

#include <ncurses.h>
#include <panel.h>

#include "world_state.hpp"

using json = nlohmann::json;

struct UIComponent {
    PANEL* panel = nullptr;
    WINDOW* window = nullptr;

    WorldState& ws;

    size_t rows;
    size_t columns;

    UIComponent(WorldState& ws, size_t rows, size_t columns, size_t begin_y, size_t begin_x);
    ~UIComponent();

    UIComponent(const UIComponent&) = delete;
    UIComponent(UIComponent&&);

    void hide();
    void show();

    struct Button {
        size_t y;
        size_t x;
        size_t height;
        size_t width;
        const std::string& name;
    };

    std::vector<Button> buttons;

    void registerButton(size_t y, size_t x, size_t height, size_t width, const std::string& name);

    void clearButtons();

    // Return the string for the button at this location, or an empty string if there is no button.
    std::string getButton(size_t y, size_t x);

    void renderDialogue(const json& dialogue);
};
