#include <ncurses.h>
#include <panel.h>

#include "world_state.hpp"
#include "uicomponent.hpp"

UIComponent::UIComponent(WorldState& ws, size_t rows, size_t columns, size_t begin_y, size_t begin_x) :
    ws(ws), rows(rows), columns(columns) {
    window = newwin(rows, columns, begin_y, begin_x);
    panel = new_panel(window);
    // No weird flush handling
    intrflush(window, false);
    // Enable keypad inputs (arrow and function keys)
    keypad(window, true);
    // Hidden by default
    hide();
}

UIComponent::~UIComponent() {
    if (nullptr != panel) {
        del_panel(panel);
    }
    if (nullptr != window) {
        delwin(window);
    }
}

UIComponent::UIComponent(UIComponent&& other) : ws(other.ws), rows(other.rows), columns(other.columns) {
    this->window = other.window;
    this->panel = other.panel;
    other.window = nullptr;
    other.panel = nullptr;
}

void UIComponent::hide() {
    hide_panel(panel);
}

void UIComponent::show() {
    show_panel(panel);
}

