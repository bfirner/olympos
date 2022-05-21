/*
 * Copyright 2022 Bernhard Firner
 *
 * Handles display formatting of different objects in the game.
 */

#include <cctype>
#include <cmath>
#include <sstream>

#include "formatting.hpp"

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

char UserInterface::getEntityChar(const Entity& ent) {
    if (ent.traits.contains("player")) {
        return '@';
    }
    else if (ent.traits.contains("wall")) {
        return '#';
    }
    else if (ent.traits.contains("mob")) {
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
    attr_t orig_attrs;
    short orig_color;
    wattr_get(window, &orig_attrs, &orig_color, nullptr);
    werase(window);
    for (const Entity& ent : entities) {
        wattr_set(window, getEntityAttr(ent), getEntityColor(ent), nullptr);
        mvwaddch(window, ent.y, ent.x, getEntityChar(ent));
    }
    // Back to the original setting
    wattr_set(window, orig_attrs, orig_color, nullptr);
}

void UserInterface::clearInput(WINDOW* window, size_t field_height, size_t field_width) {
    mvwaddch(window, field_height, 0, '>');
    for (size_t x = 1; x < field_width; ++x) {
        mvwaddch(window, field_height, x, ' ');
    }
    // Reset the cursor, leaving the '>' character to indicate where typing occurs.
    wmove(window, field_height, 1);
}

void drawString(WINDOW* window, const std::string& str) {
    // Draw the string
    for (char c : str) {
        waddch(window, c);
    }
}

void drawBar(WINDOW* window, double percent) {
    // Store the current settings
    attr_t orig_attrs;
    short orig_color;
    wattr_get(window, &orig_attrs, &orig_color, nullptr);
    for (int step = 0; step < 20; ++step) {
        short bar_color = Colors::green_on_black;
        if (step < 4) {
            bar_color = Colors::red_on_black;
        }
        else if (step < 10) {
            bar_color = Colors::yellow_on_black;
        }
        wattr_set(window, A_BOLD, bar_color, nullptr);
        if (20 * percent >= step) {
            waddch(window, '#');
        }
        else {
            waddch(window, ' ');
        }
    }
    // Restore the original settings
    wattr_set(window, orig_attrs, orig_color, nullptr);
}

void UserInterface::drawString(WINDOW* window, const std::string& str, size_t row, size_t column) {
    // Set the cursor
    wmove(window, row, column);
    drawString(window, str);
}

void UserInterface::drawStatus(WINDOW* window, const Entity& entity, size_t row, size_t column) {
    // Set the cursor
    wmove(window, row, column);
    // Draw the name
    drawString(window, entity.name);
    // Nothing more to do if there are no stats
    if (not entity.stats) {
        return;
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
    line << stats.strength << " STR # " << stats.dexterity << " DEX # " << stats.vitality << " VIT";
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

}

void UserInterface::updateEvents(WINDOW* window, std::deque<std::string>& buffer, size_t line_size) {
    // First clear the existing text from the window?
    // Now redraw the text in the event window.
    for (size_t row = 0; row < buffer.size(); ++row) {
        drawString(window, buffer[row], row, 0);
        if (buffer[row].size() < line_size) {
            std::string padding(line_size - buffer[row].size(), ' ');
            drawString(window, padding, row, buffer[row].size());
        }
    }
}
