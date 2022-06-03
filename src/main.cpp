/*
 * Copyright 2022 Bernhard Firner
 *
 * Main game setup and loop.
 */

// C headers
#include <clocale>
#include <ncurses.h>
#include <panel.h>

// C++ headers
#include <algorithm>
#include <chrono>
#include <deque>
#include <list>
#include <utility>
#include <vector>

#include "command_handler.hpp"
#include "entity.hpp"
#include "user_interface.hpp"
#include "world_state.hpp"
#include "behavior.hpp"
#include "uicomponent.hpp"

using std::string;
using std::vector;

int main(int argc, char** argv) {
    // The tick rate for the game
    // TODO Allow the user the option to set all time to their inputs.
    double tick_rate = 0.25;
    if (2 == argc) {
        tick_rate = std::stod(argv[1]);
    }

    std::setlocale(LC_ALL, "en_US.utf8");

    // Set up an ncurses screen and turn off echoing what the user writes.
    initscr();
    // Get input as it is typed.
    cbreak();
    // Don't echo what the user types.
    noecho();
    WINDOW* window = newwin(42, 80, 0, 0);
    if (has_colors()) {
        // TODO start_color() and then do stuff
        start_color();
        UserInterface::setupColors();
    }

    // No weird flush handling
    intrflush(window, false);
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

    // Get the abilities so that they can be assigned to the mobs.
    const std::vector<Behavior::AbilitySet>& abilities = Behavior::getAbilities();

    // Make some mobs
    ws.addEntity(10, 1, "Bob", {"player", "species:human", "mob"});
    // The player shouldn't have an automatic behavior set.
    ws.entities.back().behavior_set_name = "none";
    ws.addEntity(10, 10, "Spider", {"species:arachnid", "mob", "aggro", "auto"});
    ws.addEntity(10, 12, "Spider", {"species:arachnid", "mob", "aggro", "auto"});
    ws.addEntity(8, 10, "Spider", {"species:arachnid", "mob", "aggro", "auto"});
    ws.addEntity(10, 14, "Spider", {"species:arachnid", "mob", "aggro", "auto"});
    ws.addEntity(4, 6, "Bat", {"species:bat", "mob", "aggro", "auto"});
    ws.addEntity(17, 14, "Bat", {"species:bat", "mob", "aggro", "auto"});
    ws.addEntity(20, 20, "Slime", {"species:slime", "mob", "auto"});
    ws.addEntity(30, 30, "Ralph", {"species:elf", "mob", "auto"});

    // Add command handlers for all entities.
    for (Entity& entity : ws.entities) {
        for (const Behavior::AbilitySet& abset : abilities) {
            std::vector<std::string> updated = abset.updateAvailable(entity);
        }
    }

    // Turn cursor visibility to normal.
    curs_set(1);

    // Wait 50 ms for user input and then handle any of the background game stuff that should be
    // happening. Unless the user has disabled ticking.
    if (0.0 < tick_rate) {
        wtimeout(window, 50);
    }

    ws.initialize();

    // Keep an easy handle to access the player
    // TODO If we keep this here is there any reason for the world state to bother tracking some
    // named entities?
    //auto player_i = ws.named_entities["player"];
    auto player_i = ws.findEntity(std::vector<std::string>{"player"});

    std::vector<std::string> function_shortcuts;
    std::set<std::string> nav_shortcuts{"north", "east", "south", "west"};
    // Doesn't seem to be easy for someone to use a function 0 key.
    function_shortcuts.push_back("");
    for (auto& [key, value] : player_i->command_handlers) {
        if (12 > function_shortcuts.size() and not nav_shortcuts.contains(key)) {
            function_shortcuts.push_back(key);
        }
    }
    while (function_shortcuts.size() < 12) {
        function_shortcuts.push_back("");
    }

    // Create a pause panel.
    UIComponent pause_panel(ws, 38, 76, 1, 2);
    UserInterface::drawString(pause_panel.window, "===Game Paused===", 38/2, 76/2 - std::string("===Game Paused===").size()/2);

    // Create (and hide) a dialog window for help screens.
    std::map<std::string, UIComponent> help_components;
    {
        auto insert_stat = help_components.emplace(std::make_pair("help", UIComponent(ws, 38, 76, 1, 2)));
        UIComponent& uic = insert_stat.first->second;
        UserInterface::drawString(uic.window, "help", 0, 0);
        UserInterface::drawString(uic.window, "Type 'help' and a command for more information.", 2, 0);
        UserInterface::drawString(uic.window, "Available commands are:", 3, 0);
        size_t cur_row = 3;
        for (auto& [cmd_name, ability] : player_i->command_details) {
            UserInterface::drawString(uic.window, cmd_name, ++cur_row, 5);
        }
    }
    for (auto& [cmd_name, ability] : player_i->command_details) {
        // Insert a tuple for this key.
        auto insert_stat = help_components.emplace(std::make_pair(cmd_name, UIComponent(ws, 38, 76, 1, 2)));
        UIComponent& uic = insert_stat.first->second;
        // If this element already existed then clear it before drawing text.
        if (not insert_stat.second) {
            werase(uic.window);
        }
        // Set the help message.
        UserInterface::drawString(uic.window, cmd_name, 0, 0);
        UserInterface::drawString(uic.window, "Usage:", 2, 0);
        size_t cur_row = 2;
        if (0 < ability.arguments.size()) {
            if (ability.arguments.front() == "or") {
                std::string arg_str = "{";
                for (size_t idx = 1; idx < ability.arguments.size(); ++idx) {
                    arg_str = arg_str + ability.arguments.at(idx);
                    if (idx + 1 < ability.arguments.size()) {
                        arg_str += ", ";
                    }
                }
                arg_str += "}";
                UserInterface::drawString(uic.window, arg_str, ++cur_row, 5);
            }
            else {
                std::string arg_str;
                for (size_t idx = 1; idx < ability.arguments.size(); ++idx) {
                    arg_str = arg_str + ability.arguments.at(idx);
                    if (idx + 1 < ability.arguments.size()) {
                        arg_str += " ";
                    }
                }
                UserInterface::drawString(uic.window, arg_str, ++cur_row, 5);
            }
        }
        // TODO Ability type (should probably show up with the name)
        // TODO Stamina cost
        // TODO Default arguments
        // TODO Describe effects
        // TODO Add a description to the json.
    }

    // Create a panel that will be used for generic dialog.
    UIComponent dialog_box(ws, 38, 76, 1, 2);
    UserInterface::renderDialogue(dialog_box.window, "introduction", dialog_box.rows, dialog_box.columns);

    // Draw the player's status in the window
    size_t status_row = UserInterface::drawStatus(stat_window, *ws.named_entities["player"], 3, 1);
    UserInterface::drawHotkeys(stat_window, status_row+2, function_shortcuts);

    // Update the world state.
    ws.update();

    // Update panels, refresh the screen, and reset the cursor position
    UserInterface::updateDisplay(window, ws.entities);
    UserInterface::clearInput(window, ws.field_height, ws.field_width);
    update_panels();
    doupdate();

    // Initialize time stuff for the ticks
    auto last_update = std::chrono::steady_clock::now();

    bool quit = false;
    // TODO So bloated! A variable per dialog box?
    bool has_command = false;
    bool game_paused = false;
    bool in_dialog = false;
    decltype(help_components)::iterator help_displayed = help_components.end();
    std::string command = "";

    // The introduction should be displayed immediately.
    in_dialog = true;
    dialog_box.show();

    while(not quit) {
        int in_c = wgetch(window);
        std::string shortcut_str = "";
        int function_hotkey = -1;
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
            case KEY_F(0):
                function_hotkey = 0;
                break;
            case KEY_F(1):
                function_hotkey = 1;
                break;
            case KEY_F(2):
                function_hotkey = 2;
                break;
            case KEY_F(3):
                function_hotkey = 3;
                break;
            case KEY_F(4):
                function_hotkey = 4;
                break;
            case KEY_F(5):
                function_hotkey = 5;
                break;
            case KEY_F(6):
                function_hotkey = 6;
                break;
            case KEY_F(7):
                function_hotkey = 7;
                break;
            case KEY_F(8):
                function_hotkey = 8;
                break;
            case KEY_F(9):
                function_hotkey = 9;
                break;
            case KEY_F(10):
                function_hotkey = 10;
                break;
            case KEY_F(11):
                function_hotkey = 11;
                break;
            case KEY_F(12):
                function_hotkey = 12;
                break;
            case KEY_BACKSPACE:
            case 127:
                // Remove a character from the command.
                if (not command.empty()) {
                    command.pop_back();
                    wdelch(window);
                }
                in_c = ERR;
                break;
        }
        if (0 < shortcut_str.size()) {
            if (0 < command.size() and command.back() != ' ') {
                command.push_back(' ');
            }
            command += shortcut_str;
            in_c = '\n';
        }
        if (0 <= function_hotkey and 0 < function_shortcuts.at(function_hotkey).size()) {
            if (0 < command.size() and command.back() != ' ') {
                command.push_back(' ');
            }
            command += function_shortcuts.at(function_hotkey);
            // Don't process the character.
            in_c = ERR;
        }
        // Process a command on a new line.
        if ('\n' == in_c) {
            // Accept any abbreviation of quit
            if (0 < command.size() and std::string("quit").starts_with(command)) {
                quit = true;
            }
            else if (0 < command.size() and command.starts_with("help")) {
                // No pausing and helping at the same time.
                if (game_paused) {
                    game_paused = false;
                    pause_panel.hide();
                }
                // Show the top level help panel unless an argument was provided.
                std::string help_target = "help";
                if (std::string::npos != command.find_last_of(' ')) {
                    help_target = command.substr(command.find_last_of(' ')+1);
                }
                // If this help panel exists then show the panel.
                if (help_components.contains(help_target)) {
                    help_displayed = help_components.find(help_target);
                    help_displayed->second.show();
                }
            }
            else if (0 < command.size() and std::string("pause").starts_with(command)) {
                // No pausing and helping at the same time.
                if (help_displayed != help_components.end()) {
                    help_displayed->second.hide();
                    help_displayed = help_components.end();
                }
                // Pause and display the pause panel.
                game_paused = true;
                pause_panel.show();
            }
            else {
                // If the user is issuing commands then exit pause or help mode.
                if (game_paused) {
                    game_paused = false;
                    pause_panel.hide();
                }
                if (help_displayed != help_components.end()) {
                    help_displayed->second.hide();
                    help_displayed = help_components.end();
                }
                if (in_dialog) {
                    in_dialog = false;
                    dialog_box.hide();
                }
                // TODO Add user aliases.
                // Queue up actions and take them at the action tick.
                comham.enqueueNamedEntityCommand("player", command);
                has_command = true;
            }
            //Update the cursor and clear the input field
            UserInterface::clearInput(window, ws.field_height, ws.field_width);
            command = "";
        }
        else if (in_c != ERR) {
            // Since we are in noecho mode the character should be drawn.
            wechochar(window, in_c);
            command.push_back(in_c);
        }

        if (not game_paused and not in_dialog and help_displayed == help_components.end() and ws.entities.end() != player_i ) {
            auto cur_time = std::chrono::steady_clock::now();
            std::chrono::duration<double> time_diff = cur_time - last_update;
            if ((0.0 != tick_rate and tick_rate <= time_diff.count()) or
                (0.0 >= tick_rate and has_command)) {
                // Handle automated behaviors.
                const std::map<std::string, Behavior::BehaviorSet>& behaviors = Behavior::getBehaviors();
                for (Entity& entity : ws.entities) {
                    if (behaviors.contains(entity.behavior_set_name)) {
                        behaviors.at(entity.behavior_set_name).executeBehavior(entity, ws, comham);
                    }
                }
                has_command = false;
                last_update = cur_time;
                // execute all commands every tick
                comham.executeCommands(ws);
                // Tick update
                ws.update();
                // Find the user visible events.
                std::vector<std::string> player_events = ws.getLocalEvents(player_i->y, player_i->x, player_i->stats.value().detectionRange());
                for (std::string& event : player_events) {
                    event_strings.push_front(event);
                }
                // Limit to 40 events in the event window.
                while (40 < event_strings.size()) {
                    event_strings.pop_back();
                }
                // Clear the events after the user-visible ones have been dealt with.
                ws.clearEvents();
                // Draw the user visible events
                UserInterface::updateEvents(event_window, event_strings, 30);
                // Update the player's status in the window
                size_t status_row = UserInterface::drawStatus(stat_window, *ws.named_entities["player"], 3, 1);
                UserInterface::drawHotkeys(stat_window, status_row+2, function_shortcuts);
            }
        }

        // Update the player in case they have died or the trait has transferred to a new entity.
        player_i = ws.findEntity(std::vector<std::string>{"player"});
        // See if the player has died.
        if (ws.entities.end() == player_i and not in_dialog) {
            UserInterface::renderDialogue(dialog_box.window, "game over", dialog_box.rows, dialog_box.columns);
            dialog_box.show();
            in_dialog = true;
        }

        // TODO Add any visual effects that occur faster than ticks here.
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
