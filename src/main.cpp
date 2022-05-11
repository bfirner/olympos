/*
 * Copyright 2022 Bernhard Firner
 *
 * Main game setup and loop.
 */

// C headers
#include <ncurses.h>
#include <panel.h>

// C++ headers
#include <algorithm>
#include <chrono>
#include <list>

#include "command_handler.hpp"
#include "entity.hpp"
#include "formatting.hpp"
#include "world_state.hpp"
#include "behavior.hpp"

using std::string;
using std::vector;

int main(int argc, char** argv) {
    // The tick rate for the game
    // TODO Allow the user the option to set all time to their inputs.
    double tick_rate = 0.25;
    if (2 == argc) {
        tick_rate = std::stod(argv[1]);
    }

    // Set up an ncurses screen and turn off echoing what the user writes.
    // TODO Pass the window around to everything using it so that we can manage multiple windows.
    //WINDOW* window = initscr();
    initscr();
    WINDOW* window = newwin(42, 80, 0, 0);
    if (has_colors()) {
        // TODO start_color() and then do stuff
        start_color();
        UserInterface::setupColors();
    }

    // TODO keypad(stdscr) and then check for arrow keys. Maybe function keys for macros?
    // Arrow keys are KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT. Function keys are KEY_F(number)

    // Our generic command handler
    CommandHandler comham;

    // Initialize the world state with the desired size.
    WorldState ws(40, 80);

    // Create a new window to display status.
    WINDOW* stat_window = newwin(40, 30, 0, ws.field_width + 10);
    box(stat_window, 0, 0);
    UserInterface::drawString(stat_window, "Heart of Olympos", 1, 1);

    std::vector<PANEL*> panels;
    panels.push_back(new_panel(stat_window));
    panels.push_back(new_panel(window));

    // Get the behaviors so that they can be assigned to the mobs.
    const std::vector<Behavior::BehaviorSet>& behaviors = Behavior::getBehaviors();

    // Make some mobs
    ws.addEntity(10, 1, "Bob", {"player", "species:human", "mob"});
    ws.addEntity(10, 10, "Spider", {"species:arachnid", "mob", "aggro"});
    ws.addEntity(10, 12, "Spider", {"species:arachnid", "mob", "aggro"});
    ws.addEntity(8, 10, "Spider", {"species:arachnid", "mob", "aggro"});
    ws.addEntity(10, 14, "Spider", {"species:arachnid", "mob", "aggro"});
    ws.addEntity(4, 6, "Bat", {"species:bat", "mob", "aggro"});
    ws.addEntity(17, 14, "Bat", {"species:bat", "mob", "aggro"});
    ws.addEntity(20, 20, "Slime", {"species:slime", "mob", "aggro"});

    // TODO FIXME The behavior handler gets the entity passed to it in makeFunction anyway, could
    // just have the function getAvailable be "give available" or "update available" or something
    // and save us this hassle. It could still return the names of the things that are newly added.
    // This would also save the main function from carrying around the behaviors variable.
    // Add command handlers for all entities.
    for (Entity& entity : ws.entities) {
        for (const Behavior::BehaviorSet& bs : behaviors) {
            std::vector<std::string> available = bs.getAvailable(entity);
            for (const std::string& action : available) {
                entity.command_handlers.insert({action, bs.makeFunction(action, entity)});
            }
        }
    }

    ws.initialize();

    // Draw the player's status in the window
    UserInterface::drawStatus(stat_window, *ws.named_entities["player"], 3, 1);

    // Initialize time stuff for the ticks
    auto last_update = std::chrono::steady_clock::now();

    // Turn cursor visibility to normal.
    curs_set(1);

    // Wait 50 ms for user input and then handle any of the background game stuff that should be
    // happening. Unless the user has disabled ticking.
    if (0.0 < tick_rate) {
        wtimeout(window, 50);
    }

    // Update panels, refresh the screen, and reset the cursor position
    UserInterface::updateDisplay(window, ws.entities);
    UserInterface::clearInput(window, ws.field_height, ws.field_width);
    update_panels();
    doupdate();

    bool quit = false;
    bool has_command = false;
    std::string command = "";
    while(not quit) {
        char in_c = wgetch(window);
        // Process a command on a new line.
        if ('\n' == in_c) {
            // Accept any abbreviation of quit
            if (0 < command.size() and std::string("quit").starts_with(command)) {
                quit = true;
            }
            // TODO Check if numeric modifier was used
            // Queue up actions and take them at the action tick.
            comham.enqueueEntityCommand("player", command);
            //Update the cursor and clear the input field
            UserInterface::clearInput(window, ws.field_height, ws.field_width);
            command = "";
            has_command = true;
        }
        else if (in_c != ERR) {
            command.push_back(in_c);
        }
        auto cur_time = std::chrono::steady_clock::now();
        std::chrono::duration<double> time_diff = cur_time - last_update;
        if ((0.0 != tick_rate and tick_rate <= time_diff.count()) or
            (0.0 >= tick_rate and has_command)) {
            comham.enqueueTraitCommand({"small", "aggro"}, "west");
            comham.enqueueTraitCommand({"species:spider"}, "east");
            comham.enqueueTraitCommand({"species:bat"}, "south");
            has_command = false;
            last_update = cur_time;
            // execute all commands every tick
            comham.executeCommands(ws);
            ws.update();
            // Update panels, refresh the screen, and reset the cursor position
            UserInterface::updateDisplay(window, ws.entities);
            // Need to redraw the command since we've just erased the window.
            UserInterface::clearInput(window, ws.field_height, ws.field_width);
            for (char c : command) {
                waddch(window, c);
            }
            // Update panels and refresh the screen
            update_panels();
            doupdate();
        }
    }

    delwin(stat_window);
    delwin(window);
    endwin();

    return 0;
}
