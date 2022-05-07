/*
 * Copyright 2022 Bernhard Firner
 *
 * Main game setup and loop.
 */

// C headers
#include <ncurses.h>

// C++ headers
#include <algorithm>
#include <list>

#include "entity.hpp"
#include "formatting.hpp"
#include "world_state.hpp"

using std::string;
using std::vector;

void updateDisplay(const std::list<Entity>& entities) {
    for (const Entity& ent : entities) {
        mvaddch(ent.y, ent.x, getEntityChar(ent));
    }
}

void clearInput(size_t field_height, size_t field_width) {
    for (size_t x = 0; x < field_width; ++x) {
        mvaddch(field_height, x, ' ');
    }
}

int main(int argc, char** argv) {
    // Set up an ncurses screen and turn off echoing what the user writes.
    // TODO Pass the window around to everything using it so that we can manage multiple windows.
    WINDOW* window = initscr();
    if (has_colors()) {
        // TODO start_color() and then do stuff
        start_color();
    }
    // TODO all of the curses stuff should go into a renderer. The formatter could be expanded into
    // that.


    // Initialize the world state with the desired size.
    WorldState ws(40, 80);

    // Make some mobs
    ws.addEntity(10, 1, "Bob", {"player", "mob"});
    ws.addEntity(10, 10, "Spider", {"small", "mob", "aggro"});
    ws.addEntity(10, 12, "Spider", {"small", "mob", "aggro"});
    ws.addEntity(8, 10, "Spider", {"small", "mob", "aggro"});
    ws.addEntity(10, 14, "Spider", {"mob", "aggro"});
    ws.addEntity(4, 6, "Bat", {"small", "flying", "mob", "aggro"});
    ws.addEntity(17, 14, "Bat", {"small", "flying", "mob", "aggro"});

    ws.initialize();

    // Turn cursor visibility to normal.
    curs_set(1);

    // Update the display and put the user input outside of the field.
    updateDisplay(ws.entities);
    move(ws.field_height, 0);

    bool quit = false;
    std::string command = "";
    while(not quit) {
        char in_c = getch();
        // Process a command on a new line.
        if ('\n' == in_c) {
            // Accept any abbreviation of quit
            if (std::string("quit").starts_with(command)) {
                quit = true;
            }
            // TODO Set up command handlers
            // TODO Check if numeric modifier was used
            // TODO Queue actions and take them all at every tick of the clock
            auto player = ws.named_entities["player"];
            if (std::string("north").starts_with(command)) {
                if (ws.passable[player->y-1][player->x]) {
                    player->y -= 1;
                }
            }
            if (std::string("east").starts_with(command)) {
                if (ws.passable[player->y][player->x+1]) {
                    player->x += 1;
                }
            }
            if (std::string("south").starts_with(command)) {
                if (ws.passable[player->y+1][player->x]) {
                    player->y += 1;
                }
            }
            if (std::string("west").starts_with(command)) {
                if (ws.passable[player->y][player->x-1]) {
                    player->x -= 1;
                }
            }
            //Update the cursor and clear the input field
            move(ws.field_height, 0);
            clearInput(ws.field_height, ws.field_width);
            command = "";
        }
        else {
            command.push_back(in_c);
        }
        // Erase the window before redrawing.
        werase(window);
        updateDisplay(ws.entities);
        move(ws.field_height, 0);
        // Need to redraw the command since we've just erased the window.
        for (char c : command) {
            addch(c);
        }
        ws.update();
    }

    endwin();

    return 0;
}
