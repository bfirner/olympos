/*
 * Copyright 2022 Bernhard Firner
 *
 * A UI component.
 */

#pragma once

#include <ncurses.h>
#include <panel.h>

#include "world_state.hpp"

struct UIComponent {
    PANEL* panel = nullptr;
    WINDOW* window = nullptr;

    size_t rows;
    size_t columns;

    WorldState& ws;

    UIComponent(WorldState& ws, size_t rows, size_t columns, size_t begin_y, size_t begin_x);
    ~UIComponent();

    UIComponent(const UIComponent&) = delete;
    UIComponent(UIComponent&&);

    void hide();
    void show();
};
