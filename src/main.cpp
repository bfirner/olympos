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
#include <deque>
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

    // Enable keypad inputs (arrow and function keys)
    keypad(window, true);

    // Our generic command handler
    CommandHandler comham;

    // Initialize the world state with the desired size.
    WorldState ws(40, 80);

    // Create a new window to display status.
    WINDOW* stat_window = newwin(40, 30, 0, ws.field_width + 10);
    box(stat_window, 0, 0);
    UserInterface::drawString(stat_window, "Heart of Olympos", 1, 1);

    // Create another window for the event log.
    WINDOW* event_window = newwin(40, 80, 41, 0);
    std::deque<std::string> event_strings;

    std::vector<PANEL*> panels;
    panels.push_back(new_panel(event_window));
    panels.push_back(new_panel(stat_window));
    panels.push_back(new_panel(window));

    // Get the behaviors so that they can be assigned to the mobs.
    const std::vector<Behavior::BehaviorSet>& behaviors = Behavior::getBehaviors();

    // Make some mobs
    ws.addEntity(10, 1, "Bob", {"player", "species:human", "mob"});
    ws.addEntity(10, 10, "Spider", {"species:arachnid", "mob", "aggro", "auto"});
    ws.addEntity(10, 12, "Spider", {"species:arachnid", "mob", "aggro", "auto"});
    ws.addEntity(8, 10, "Spider", {"species:arachnid", "mob", "aggro", "auto"});
    ws.addEntity(10, 14, "Spider", {"species:arachnid", "mob", "aggro", "auto"});
    ws.addEntity(4, 6, "Bat", {"species:bat", "mob", "aggro", "auto"});
    ws.addEntity(17, 14, "Bat", {"species:bat", "mob", "aggro", "auto"});
    ws.addEntity(20, 20, "Slime", {"species:slime", "mob", "auto"});

    // Add command handlers for all entities.
    for (Entity& entity : ws.entities) {
        for (const Behavior::BehaviorSet& bs : behaviors) {
            std::vector<std::string> updated = bs.updateAvailable(entity);
        }
    }

    ws.initialize();

    // Draw the player's status in the window
    UserInterface::drawStatus(stat_window, *ws.named_entities["player"], 3, 1);

    // Update the world state.
    ws.update();

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

    // Initialize time stuff for the ticks
    auto last_update = std::chrono::steady_clock::now();

    bool quit = false;
    bool has_command = false;
    std::string command = "";
    while(not quit) {
        int in_c = wgetch(window);
        std::string shortcut_str = "";
        switch (in_c) {
            // Check for arrow keys and treat them as an immediately commanded direction and enter
            // key.
            case KEY_UP:
                shortcut_str = "north";
                break;
            case KEY_DOWN:
                shortcut_str = "south";
                break;
            case KEY_LEFT:
                shortcut_str = "west";
                break;
            case KEY_RIGHT:
                shortcut_str = "east";
                break;
        }
        if (0 < shortcut_str.size()) {
            if (0 < command.size() and command.back() != ' ') {
                command.push_back(' ');
            }
            command += shortcut_str;
            in_c = '\n';
        }
        // Process a command on a new line.
        if ('\n' == in_c) {
            // Accept any abbreviation of quit
            if (0 < command.size() and std::string("quit").starts_with(command)) {
                quit = true;
            }
            // TODO Add user aliases.
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
            comham.enqueueTraitCommand({"species:slime"}, "wander");
            has_command = false;
            last_update = cur_time;
            // execute all commands every tick
            comham.executeCommands(ws);
            auto player_i = ws.named_entities["player"];
            // Find the user visible events.
            std::vector<std::string> player_events = ws.getLocalEvents(player_i->y, player_i->x, 10);
            for (std::string& event : player_events) {
                event_strings.push_front(event);
            }
            // Limit to 40 events in the event window.
            while (40 < event_strings.size()) {
                event_strings.pop_back();
            }
            // Clear the events after the user-visible ones have been dealt with.
            ws.clearEvents();
            ws.update();
            UserInterface::updateEvents(event_window, event_strings, 30);
            // Update the player's status in the window
            UserInterface::drawStatus(stat_window, *ws.named_entities["player"], 3, 1);
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

    // Clean things up.
    for (PANEL* panel : panels) {
        del_panel(panel);
    }
    delwin(event_window);
    delwin(stat_window);
    delwin(window);
    endwin();

    return 0;
}
