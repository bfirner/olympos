/*
 * Copyright 2022 Bernhard Firner
 *
 * Handles display formatting of different objects in the game.
 */

#include <nlohmann/json.hpp>

#include <cctype>
#include <cmath>
#include <cwchar>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <clocale>
#include <regex>
#include <sstream>
#include <tuple>

using json = nlohmann::json;

#include "olympos_utility.hpp"
#include "user_interface.hpp"


json json_dialogue;

json& getDialogue() {
    // Read in the file if it hasn't already been done.
    const std::filesystem::path dialogue_path{"resources/dialogue.json"};
    if (0 == json_dialogue.size()) {
        if (std::filesystem::exists(dialogue_path)) {
            std::ifstream istream(dialogue_path.string(), std::ios::binary);
            if (istream) {
                std::string contents;
                std::getline(istream, contents, '\0');
                json_dialogue = json::parse(contents);
            }
        }
    }
    return json_dialogue;
}

/*
 COLOR_BLACK
 COLOR_RED
 COLOR_GREEN
 COLOR_YELLOW
 COLOR_BLUE
 COLOR_MAGENTA
 COLOR_CYAN
 COLOR_WHITE


VIDEO ATTRIBUTES
       The following video attributes, defined in <curses.h>, can be passed to the routines attron, attroff, and attrset, or OR'd with the characters passed to  addch  (see
       curs_addch(3X)).

              Name           Description
              ─────────────────────────────────────────────────────────────────
              A_NORMAL       Normal display (no highlight)
              A_STANDOUT     Best highlighting mode of the terminal.
              A_UNDERLINE    Underlining
              A_REVERSE      Reverse video
              A_BLINK        Blinking
              A_DIM          Half bright
              A_BOLD         Extra bright or bold
              A_PROTECT      Protected mode
              A_INVIS        Invisible or blank mode
              A_ALTCHARSET   Alternate character set
              A_ITALIC       Italics (non-X/Open extension)
              A_CHARTEXT     Bit-mask to extract a character
              A_COLOR        Bit-mask to extract a color (legacy routines)

       These video attributes are supported by attr_on and related functions (which also support the attributes recognized by attron, etc.):

              Name            Description
              ─────────────────────────────────────────
              WA_HORIZONTAL   Horizontal highlight
              WA_LEFT         Left highlight
              WA_LOW          Low highlight
              WA_RIGHT        Right highlight
              WA_TOP          Top highlight
              WA_VERTICAL     Vertical highlight
*/

std::wstring UserInterface::getEntityChar(const Entity& ent) {
    if (ent.traits.contains("player")) {
        return L"@";
    }
    else if (ent.traits.contains("wall")) {
        return L"#";
    }
    else {
        return ent.character;
    }
}

attr_t UserInterface::getEntityAttr(const Entity& ent) {
    if (ent.traits.contains("player")) {
        return A_BOLD;
    }
    else if (ent.traits.contains("wall")) {
        return A_DIM;
    }
    else if (ent.traits.contains("aggro")) {
        return A_BOLD;
    }
    // Default is normal
    return A_NORMAL;
}

enum Colors : short {
    white_on_black = 1,
    blue_on_black = 2,
    red_on_black = 3,
    green_on_black = 4,
    yellow_on_black = 5,
    magenta_on_black = 6,
    cyan_on_black = 7,
};

short UserInterface::getEntityColor(const Entity& ent) {
    if (ent.traits.contains("aggro")) {
        return Colors::red_on_black;
    }
    // Default is white
    return Colors::white_on_black;
}

std::tuple<attr_t, Colors> strToAttrCode(const std::string& color) {
    // Default to white in case this isn't a recognized color.
    Colors the_color = white_on_black;
    if ("blue" == color) {
        the_color = blue_on_black;
    }
    else if ("red" == color) {
        the_color = red_on_black;
    }
    else if ("green" == color) {
        the_color = green_on_black;
    }
    else if ("yellow" == color) {
        the_color = yellow_on_black;
    }
    else if ("magenta" == color) {
        the_color = magenta_on_black;
    }
    else if ("cyan" == color) {
        the_color = cyan_on_black;
    }

    // TODO Defaulting to normal text for now.
    return std::make_tuple(A_NORMAL, the_color);
}

void UserInterface::setupColors() {
    init_pair(Colors::white_on_black, COLOR_WHITE, COLOR_BLACK);
    init_pair(Colors::blue_on_black, COLOR_BLUE, COLOR_BLACK);
    init_pair(Colors::red_on_black, COLOR_RED, COLOR_BLACK);
    init_pair(Colors::green_on_black, COLOR_GREEN, COLOR_BLACK);
    init_pair(Colors::yellow_on_black, COLOR_YELLOW, COLOR_BLACK);
    init_pair(Colors::magenta_on_black, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(Colors::cyan_on_black, COLOR_CYAN, COLOR_BLACK);
}

void UserInterface::updateDisplay(WINDOW* window, const std::list<Entity>& entities) {
    // Store the original colors so that they can be easily restored.
    attr_t orig_attrs;
    short orig_color;
    wattr_get(window, &orig_attrs, &orig_color, nullptr);
    werase(window);
    for (const Entity& ent : entities) {
        wattr_set(window, getEntityAttr(ent), getEntityColor(ent), nullptr);
        //mvwaddch(window, ent.y, ent.x, getEntityChar(ent));
        drawString(window, getEntityChar(ent), ent.y, ent.x);
    }
    // Back to the original setting
    wattr_set(window, orig_attrs, orig_color, nullptr);
}

void UserInterface::clearInput(WINDOW* window, size_t field_height, size_t field_width) {
    std::string line = ">" + std::string(field_width-1, ' ');
    mvwprintw(window, field_height, 0, "%s", line.c_str());
    // Reset the cursor, leaving the '>' character to indicate where typing occurs.
    wmove(window, field_height, 1);
}

void drawString(WINDOW* window, const std::wstring& str) {
    // Draw the string
    wprintw(window, "%ls", str.data());
}

void drawString(WINDOW* window, const std::string& str, const std::string& color) {
    // Store the original colors so that they can be easily restored.
    attr_t orig_attrs;
    short orig_color;
    wattr_get(window, &orig_attrs, &orig_color, nullptr);

    // Switch to the desired color
    auto [attr_code, color_code] = strToAttrCode(color);
    wattr_set(window, attr_code, color_code, nullptr);

    // Draw the string
    wprintw(window, "%s", str.data());

    // Reset the color
    wattr_set(window, orig_attrs, orig_color, nullptr);
}

void drawString(WINDOW* window, const std::wstring& str, const std::string& color) {
    // Store the original colors so that they can be easily restored.
    attr_t orig_attrs;
    short orig_color;
    wattr_get(window, &orig_attrs, &orig_color, nullptr);

    // Switch to the desired color
    auto [attr_code, color_code] = strToAttrCode(color);
    wattr_set(window, attr_code, color_code, nullptr);

    // Draw the string
    wprintw(window, "%ls", str.data());

    // Reset the color
    wattr_set(window, orig_attrs, orig_color, nullptr);
}

void drawString(WINDOW* window, const std::string& str) {
    // Draw the string
    for (char c : str) {
        waddch(window, c);
    }
}

void UserInterface::drawString(WINDOW* window, const std::wstring& str, size_t row, size_t column) {
    mvwprintw(window, row, column, "%ls", str.data());
}

void UserInterface::drawString(WINDOW* window, const std::string& str, size_t row, size_t column) {
    // Set the cursor
    wmove(window, row, column);
    drawString(window, str);
}

void drawBar(WINDOW* window, double percent) {
    // Store the current settings
    attr_t orig_attrs;
    short orig_color = 0;
    wattr_get(window, &orig_attrs, &orig_color, nullptr);
    for (int step = 0; step < 20; ++step) {
        short bar_color = Colors::green_on_black;
        const std::wstring full_box = L"■";
        const std::wstring half_box = L"□";
        if (step < 4) {
            bar_color = Colors::red_on_black;
        }
        else if (step < 10) {
            bar_color = Colors::yellow_on_black;
        }
        wattr_set(window, A_BOLD, bar_color, nullptr);
        // At or above this 5% increment
        if (20 * percent >= step) {
            drawString(window, full_box);
        }
        // Between the previous increment and this one.
        else if (20 * percent > (step - 1)) {
            drawString(window, half_box);
        }
        else {
            waddch(window, ' ');
        }
    }
    // Restore the original settings
    wattr_set(window, orig_attrs, orig_color, nullptr);
}

size_t UserInterface::drawStatus(WINDOW* window, const Entity& entity, size_t row, size_t column) {
    // Set the cursor
    wmove(window, row, column);
    // Draw the name
    drawString(window, entity.name);
    // Nothing more to do if there are no stats
    if (not entity.stats) {
        return row;
    }
    // Otherwise continue with the species.
    drawString(window, " (" + entity.getSpecies() + ")");
    const Stats& stats = entity.stats.value();
    std::ostringstream line;

    // TODO Colors on hp and mana
    int cur_row = row+1;

    // HP Information
    wmove(window, cur_row++, column);
    line << "[HP] " << stats.health << "/" << stats.maxHealth();
    drawString(window, line.str());
    wmove(window, cur_row++, column);
    drawBar(window, static_cast<double>(stats.health) / stats.maxHealth());

    // Mana information
    wmove(window, cur_row++, column);
    line.str("");
    line << "[MP] " << stats.mana << "/" << stats.maxMana();
    drawString(window, line.str());
    wmove(window, cur_row++, column);
    drawBar(window, static_cast<double>(stats.mana) / stats.mana);

    // Stamina information
    wmove(window, cur_row++, column);
    line.str("");
    line << "[Stamina] " << stats.stamina << "/" << stats.maxStamina();
    drawString(window, line.str());
    wmove(window, cur_row++, column);
    drawBar(window, static_cast<double>(stats.stamina) / stats.maxStamina());

    // Physical status
    wmove(window, cur_row++, column);
    drawString(window, "[Physical]");
    wmove(window, cur_row++, column);
    line.str("");
    line << stats.strength << " STR # " << stats.reflexes << " RFLX # " << stats.vitality << " VIT";
    drawString(window, line.str());

    // Metaphysical status
    wmove(window, cur_row++, column);
    drawString(window, "[Metaphysical]");
    line.str("");
    line << stats.aura << " AURA # " << stats.domain << " DOMAIN";
    wmove(window, cur_row++, column);
    drawString(window, line.str());
    line.str("");
    line << stats.channel_rate << " channel rate";
    wmove(window, cur_row++, column);
    drawString(window, line.str());

    // TODO Classes
    wmove(window, cur_row++, column);
    drawString(window, "Description:");
    std::string description = entity.getDescription();
    wmove(window, cur_row, column);
    drawString(window, std::string(28, ' '));
    wmove(window, cur_row, column);
    drawString(window, description);

    return cur_row;
}

void UserInterface::updateEvents(WINDOW* window, std::deque<std::string>& buffer) {
    werase(window);
    // Redraw the text in the event window, padding out to the end of the line.
    for (size_t row = 0; row < buffer.size(); ++row) {
        // Get the cursor to the correct location.
        wmove(window, row, 0);
        // Add support for color commands. Tags are stored in [] and the target string follows in ()
        // Trying to match strings like this: "<entity> [color:red](kicks) <target>."
        const std::regex color_tags("\\[color:([a-z]+)\\]\\(([[:alnum:]]+)\\)");

        // Need to hold on to the unmatched portion of the string.
        std::string rest = buffer[row];
        size_t printed_chars = 0;
        std::smatch matches;
        while (std::regex_search(rest, matches, color_tags)) {
            std::string comparison = matches[1].str();
            // Print the prefix.
            drawString(window, matches.prefix().str());
            // Get the color.
            const std::string& color = matches[1].str();
            // Print the match.
            drawString(window, matches[2].str(), color);
            // Set rest to the rest of the string
            rest = matches.suffix().str();
            printed_chars += matches.prefix().str().size() + matches[2].str().size();
        }

        // Now print the unmatched part of the string.
        drawString(window, rest);

        /*
        if (buffer[row].size() < line_size) {
            std::string padding(line_size - printed_chars, ' ');
            drawString(window, padding);
        }
        */
    }
}

size_t UserInterface::drawHotkeys(WINDOW* window, size_t row, const std::vector<std::string>& shortcuts) {
    // Label
    drawString(window, "Hotkeys:", row, 1);
    row += 1;

    // Navigation
    drawString(window, L"↑) north", ++row, 1);
    drawString(window, L"→) east", ++row, 1);
    drawString(window, L"↓) south", ++row, 1);
    drawString(window, L"←) west", ++row, 1);

    // Function hotkeys
    for (size_t idx = 0; idx < shortcuts.size(); ++idx) {
        if (0 < shortcuts[idx].size()) {
            drawString(window, "F" + std::to_string(idx) + ") " + shortcuts[idx], ++row, 1);
        }
    }
    return row;
}

void drawPause(WINDOW* window, size_t rows, size_t columns) {
    const std::string message = "===PAUSED===";
    // Clear the window, then draw the pause message.
    werase(window);
    mvwprintw(window, rows / 2, columns / 2 - message.size() / 2, "%s", message.data());
}

bool UserInterface::hasDialogue(const std::string& dialogue_name) {
    json& dialogue = getDialogue();
    return dialogue.contains(dialogue_name);
}

void UserInterface::renderDialogue(WINDOW* window, const std::string& dialogue_name, size_t rows, size_t columns) {
    json& dialogue = getDialogue();

    // Clear the window and render the text as instructed in the json.
    werase(window);

    if (not dialogue.contains(dialogue_name)) {
        // TODO FIXME Some kind of error is required here.
        return;
    }

    std::vector<std::wstring> text;
    // Get the text and convert it to wstring format.
    {
        json& jtext = dialogue.at(dialogue_name).at("text");
        for (const std::string line : jtext) {
            text.push_back(OlymposUtility::utf8ToWString(line));
        }
    }

    std::string placement = dialogue.at(dialogue_name).at("placement");

    // Determine where the text should be drawn.
    // Text the begins in the upper left.
    if ("upper left" == placement) {
        size_t cur_row = 0;
        // Draw the text
        for (const std::wstring w_line : text) {
            drawString(window, w_line, cur_row++, 0);
        }
    }
    else if ("centered" == placement) {
        size_t cur_row = 0;
        // Draw the text
        for (const std::wstring w_line : text) {
            size_t y = cur_row + (rows - text.size()) / 2;
            size_t x = (columns - w_line.size()) / 2;
            drawString(window, w_line, y, x);
            ++cur_row;
        }
    }

    // User options, draw at the bottom of the window.
    if (dialogue.at(dialogue_name).contains("options")) {
        // TODO FIXME Need some kind of callback here I guess.
        json& options = dialogue.at(dialogue_name).at("options");

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
        drawString(window, top_line, rows-3, col_start);
        drawString(window, middle_line, rows-2, col_start);
        drawString(window, bottom_line, rows-1, col_start);
    }
}
