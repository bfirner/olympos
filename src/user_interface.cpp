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

json& getDialogueJson() {
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
    else {
        return ent.character;
    }
}

attr_t UserInterface::getEntityAttr(const Entity& ent) {
    if (ent.traits.contains("player")) {
        return A_BOLD;
    }
    else if (ent.traits.contains("impassable")) {
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
    white_on_cyan,
    blue_on_cyan,
    red_on_cyan,
    green_on_cyan,
    yellow_on_cyan,
    magenta_on_cyan,
    cyan_on_cyan,
    white_on_red,
    blue_on_red,
    red_on_red,
    green_on_red,
    yellow_on_red,
    magenta_on_red,
    cyan_on_red,
    white_on_green,
    blue_on_green,
    red_on_green,
    green_on_green,
    yellow_on_green,
    magenta_on_green,
    cyan_on_green,
};

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
    else if ("white on cyan" == color) {
        the_color = white_on_cyan;
    }
    else if ("blue on cyan" == color) {
        the_color = blue_on_cyan;
    }
    else if ("red on cyan" == color) {
        the_color = red_on_cyan;
    }
    else if ("green on cyan" == color) {
        the_color = green_on_cyan;
    }
    else if ("yellow on cyan" == color) {
        the_color = yellow_on_cyan;
    }
    else if ("magenta on cyan" == color) {
        the_color = magenta_on_cyan;
    }
    else if ("cyan on cyan" == color) {
        the_color = cyan_on_cyan;
    }
    else if ("white on red" == color) {
        the_color = white_on_red;
    }
    else if ("blue on red" == color) {
        the_color = blue_on_red;
    }
    else if ("red on red" == color) {
        the_color = red_on_red;
    }
    else if ("green on red" == color) {
        the_color = green_on_red;
    }
    else if ("yellow on red" == color) {
        the_color = yellow_on_red;
    }
    else if ("magenta on red" == color) {
        the_color = magenta_on_red;
    }
    else if ("cyan on red" == color) {
        the_color = cyan_on_red;
    }
    else if ("white on green" == color) {
        the_color = white_on_green;
    }
    else if ("blue on green" == color) {
        the_color = blue_on_green;
    }
    else if ("red on green" == color) {
        the_color = red_on_green;
    }
    else if ("green on green" == color) {
        the_color = green_on_green;
    }
    else if ("yellow on green" == color) {
        the_color = yellow_on_green;
    }
    else if ("magenta on green" == color) {
        the_color = magenta_on_green;
    }
    else if ("cyan on green" == color) {
        the_color = cyan_on_green;
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

    init_pair(Colors::white_on_cyan, COLOR_WHITE, COLOR_CYAN);
    init_pair(Colors::blue_on_cyan, COLOR_BLUE, COLOR_CYAN);
    init_pair(Colors::red_on_cyan, COLOR_RED, COLOR_CYAN);
    init_pair(Colors::green_on_cyan, COLOR_GREEN, COLOR_CYAN);
    init_pair(Colors::yellow_on_cyan, COLOR_YELLOW, COLOR_CYAN);
    init_pair(Colors::magenta_on_cyan, COLOR_MAGENTA, COLOR_CYAN);
    init_pair(Colors::cyan_on_cyan, COLOR_CYAN, COLOR_CYAN);

    init_pair(Colors::white_on_red, COLOR_WHITE, COLOR_RED);
    init_pair(Colors::blue_on_red, COLOR_BLUE, COLOR_RED);
    init_pair(Colors::red_on_red, COLOR_RED, COLOR_RED);
    init_pair(Colors::green_on_red, COLOR_GREEN, COLOR_RED);
    init_pair(Colors::yellow_on_red, COLOR_YELLOW, COLOR_RED);
    init_pair(Colors::magenta_on_red, COLOR_MAGENTA, COLOR_RED);
    init_pair(Colors::cyan_on_red, COLOR_CYAN, COLOR_RED);

    init_pair(Colors::white_on_green, COLOR_WHITE, COLOR_GREEN);
    init_pair(Colors::blue_on_green, COLOR_BLUE, COLOR_GREEN);
    init_pair(Colors::red_on_green, COLOR_RED, COLOR_GREEN);
    init_pair(Colors::green_on_green, COLOR_GREEN, COLOR_GREEN);
    init_pair(Colors::yellow_on_green, COLOR_YELLOW, COLOR_GREEN);
    init_pair(Colors::magenta_on_green, COLOR_MAGENTA, COLOR_GREEN);
    init_pair(Colors::cyan_on_green, COLOR_CYAN, COLOR_GREEN);
}

short UserInterface::getEntityColor(const Entity& ent, const std::string& bg_color) {
    if (ent.traits.contains("aggro")) {
        if ("black" == bg_color) {
            return Colors::red_on_black;
        }
        else {
            return std::get<1>(strToAttrCode("red on " + bg_color));
        }
    }
    else if ("black" != bg_color) {
        return std::get<1>(strToAttrCode("white on " + bg_color));
    }
    // Default is white
    return Colors::white_on_black;
}

void UserInterface::updateDisplay(WINDOW* window, const std::list<Entity>& entities,
        const std::map<std::tuple<size_t, size_t>, std::string>& background_effects) {
    // Store the original colors so that they can be easily restored.
    attr_t orig_attrs;
    short orig_color;
    wattr_get(window, &orig_attrs, &orig_color, nullptr);
    werase(window);
    // Print all of the entities then add in effects. Don't let an effect overwrite an entity.
    std::set<std::tuple<size_t, size_t>> drawn;
    for (const Entity& ent : entities) {
        auto location = std::make_tuple(ent.y, ent.x);
        if (not background_effects.contains(location)) {
            wattr_set(window, getEntityAttr(ent), getEntityColor(ent), nullptr);
        }
        else {
            wattr_set(window, getEntityAttr(ent), getEntityColor(ent, background_effects.at(location)), nullptr);
        }
        //mvwaddch(window, ent.y, ent.x, getEntityChar(ent));
        drawString(window, getEntityChar(ent), ent.y, ent.x);

        drawn.insert({ent.y, ent.x});
    }
    // Now draw the background effects for any locations not already drawn.
    for (auto& [location, color] : background_effects) {
        if (not drawn.contains(location)) {
            wattr_set(window, A_NORMAL, std::get<1>(strToAttrCode("white on " + color)), nullptr);
            drawString(window, " ", std::get<0>(location), std::get<1>(location));
        }
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
    werase(window);
    // TODO This should just be a specialization of UIComponent so there isn't hidden knowledge
    // inside of the function.
    box(window, 0, 0);
    UserInterface::drawString(window, "Heart of Olympos", 1, 1);

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

size_t UserInterface::drawInfolog(WINDOW* window, size_t row, std::deque<std::vector<std::wstring>> info_log) {
    // Section Label
    // TODO Use actual lines to subdivide the event window.
    drawString(window, "Information:", row, 1);
    row += 1;

    for (std::vector<std::wstring>& info : info_log) {
        for (std::wstring& line : info) {
            drawString(window, line, row, 1);
            // This string could have actually taken up more than one line, so update the position
            // variable.
            row = getcury(window) + 1;
        }
        // Leave a space between different info log messages.
        row = getcury(window) + 2;
    }
    return row;
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

bool UserInterface::hasDialogue(const std::string& dialogue_name) {
    json& dialogue = getDialogueJson();
    return dialogue.contains(dialogue_name);
}

json& UserInterface::getDialogue(const std::string& dialogue_name) {
    json& dialogue = getDialogueJson();
    return dialogue.at(dialogue_name);
}
