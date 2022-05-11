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
    //wrefresh(window);
}

void UserInterface::clearInput(WINDOW* window, size_t field_height, size_t field_width) {
    for (size_t x = 0; x < field_width; ++x) {
        mvwaddch(window, field_height, x, ' ');
    }
    // Reset the cursor
    wmove(window, field_height, 0);
}

void drawString(WINDOW* window, const std::string& str) {
    // Draw the string
    for (char c : str) {
        waddch(window, c);
    }
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
    // HP Information
    wmove(window, row+1, column);
    line << "[HP] " << stats.health << "/" << std::floor(stats.vitality*0.8 + stats.domain*0.2);
    drawString(window, line.str());

    // Mana information
    wmove(window, row+2, column);
    line.str("");
    line << "[MP] " << stats.mana << "/" << stats.pool_volume;
    drawString(window, line.str());
    line.str("");
    line << stats.channel_rate << " channel rate";
    wmove(window, row+3, column);
    drawString(window, line.str());

    // Physical status
    wmove(window, row+5, column);
    drawString(window, "[Physical]");
    wmove(window, row+6, column);
    line.str("");
    line << stats.power << " POW # " << stats.dexterity << " DEX # " << stats.vitality << " VIT";
    drawString(window, line.str());

    // Metaphysical status
    wmove(window, row+8, column);
    drawString(window, "[Metaphysical]");
    line.str("");
    line << stats.aura << " AURA # " << stats.domain << " DOMAIN";
    wmove(window, row+9, column);
    drawString(window, line.str());

    // TODO Classes
}
