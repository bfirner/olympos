#include <ncurses.h>
#include <panel.h>

#include "olympos_utility.hpp"
#include "world_state.hpp"
#include "uicomponent.hpp"
#include "user_interface.hpp"

UIComponent::UIComponent(size_t rows, size_t columns, size_t begin_y, size_t begin_x) :
    rows(rows), columns(columns) {
    window = newwin(rows, columns, begin_y, begin_x);
    panel = new_panel(window);
    // No weird flush handling
    intrflush(window, false);
    // Enable keypad inputs (arrow and function keys)
    keypad(window, true);
    // Hidden on the bottom by default
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

UIComponent::UIComponent(UIComponent&& other) : rows(other.rows), columns(other.columns) {
    this->window = other.window;
    this->panel = other.panel;
    other.window = nullptr;
    other.panel = nullptr;
}

void UIComponent::hide() {
    bottom_panel(panel);
    hide_panel(panel);
}

void UIComponent::show() {
    top_panel(panel);
    show_panel(panel);
}

void UIComponent::registerButton(size_t y, size_t x, size_t height, size_t width, const std::string& name) {
    buttons.push_back({y, x, height, width, name});
}

void UIComponent::clearButtons() {
    buttons.clear();
}

// Return the string for the button at this location, or an empty string if there is no button.
std::string UIComponent::getButton(size_t y, size_t x) {
    for (Button& button : buttons) {
        if (button.y <= y and button.x <= x and button.y + button.height >= y and button.x + button.width >= x) {
            return button.name;
        }
    }
    return "";
}

void UIComponent::renderDialogue(const json& dialogue) {
    // TODO Mark the button locations.

    // Clear the window and render the text as instructed in the json.
    werase(window);
    clearButtons();

    std::vector<std::wstring> text;
    // Get the text and convert it to wstring format.
    {
        json jtext = dialogue.at("text");
        for (const std::string line : jtext) {
            text.push_back(OlymposUtility::utf8ToWString(line));
        }
    }

    std::string placement = dialogue.at("placement");

    // Determine where the text should be drawn.
    // Text the begins in the upper left.
    if ("upper left" == placement) {
        size_t cur_row = 0;
        // Draw the text
        for (const std::wstring& w_line : text) {
            UserInterface::drawString(window, w_line, cur_row++, 0);
        }
    }
    else if ("centered" == placement) {
        size_t cur_row = 0;
        // Draw the text
        for (const std::wstring& w_line : text) {
            size_t y = cur_row + (rows - text.size()) / 2;
            size_t x = (columns - w_line.size()) / 2;
            UserInterface::drawString(window, w_line, y, x);
            ++cur_row;
        }
    }

    // User options, draw at the bottom of the window.
    if (dialogue.contains("options")) {
        // TODO FIXME Need some kind of callback here I guess.
        json options = dialogue.at("options");

        // Draw boxes around each option
        // That means a top line, a bottom line, and the options in the middle.
        const wchar_t upper_left = L'╭';
        const wchar_t lower_left = L'╰';
        const wchar_t upper_right = L'╮';
        const wchar_t lower_right = L'╯';
        const wchar_t top_partition    = L'┬';
        const wchar_t bottom_partition = L'┴';
        const wchar_t horizontal = L'─';
        const wchar_t vertical = L'│';

        std::wstring top_line(1, upper_left);
        std::wstring middle_line(1, vertical);
        std::wstring bottom_line(1, lower_left);

        // TODO Cannot parse wstring from nlohmann::json
        for (const std::string option : options) {
            std::wstring w_option = OlymposUtility::utf8ToWString(option);

            // Add a partition if this wasn't the first option
            if (1 != top_line.size()) {
                top_line = top_line + top_partition;
                middle_line = middle_line + vertical;
                bottom_line = bottom_line + bottom_partition;
            }
            top_line = top_line + std::wstring(w_option.size() + 2, horizontal);
            middle_line = middle_line + L" " + w_option + L" ";
            bottom_line = bottom_line + std::wstring(w_option.size() + 2, horizontal);
        }
        // Close the box
        top_line = top_line + upper_right;
        middle_line = middle_line + vertical;
        bottom_line = bottom_line + lower_right;

        // Now draw the options
        size_t col_start = columns / 2 - top_line.size() / 2;
        UserInterface::drawString(window, top_line, rows-3, col_start);
        UserInterface::drawString(window, middle_line, rows-2, col_start);
        UserInterface::drawString(window, bottom_line, rows-1, col_start);

        // Register the buttons to handle mouse clicks later.
        size_t button_offset = 0;
        for (const std::string option : options) {
            registerButton(rows-3, col_start + button_offset, 3, option.size()+2, option);
            button_offset += 2;
        }
    }
}
