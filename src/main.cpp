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
#include <iostream>
#include <list>
#include <regex>
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
    size_t main_window_height = 42;
    WINDOW* window = newwin(main_window_height, 80, 0, 0);
    if (has_colors()) {
        // TODO start_color() and then do stuff
        start_color();
        UserInterface::setupColors();
    }

    // Enable mouse button press and release events
    if (has_mouse()) {
        // TODO Check for success or failure
        mousemask(BUTTON1_CLICKED, nullptr);
    }

    // No weird flush handling
    intrflush(window, false);
    // Enable keypad inputs (arrow and function keys)
    keypad(window, true);

    // Our generic command handler
    CommandHandler comham;

    // Initialize the world state with the desired size.
    WorldState ws(40, 80);

    // Get the abilities so that they can be assigned to the mobs.
    const std::vector<Behavior::AbilitySet>& abilities = Behavior::getAbilities();

    // Make some mobs
    ws.addEntity(10, 1, "Bob", {"player", "species:human", "mob"});
    // The player shouldn't have an automatic behavior set.
    ws.entities.back().behavior_set_name = "none";
    ws.addEntity(10, 10, "Blue Slime", {"species:slime", "mob", "auto"});
    ws.addEntity(10, 12, "Green Slime", {"species:slime", "mob", "auto"});
    ws.addEntity(8, 10, "Purple Slime", {"species:slime", "mob", "auto"});
    ws.addEntity(10, 14, "Jiggling Slime", {"species:slime", "mob", "auto"});
    ws.addEntity(4, 6, "Bat", {"species:bat", "mob", "aggro", "auto"});
    ws.addEntity(17, 14, "Bat", {"species:bat", "mob", "aggro", "auto"});
    ws.addEntity(20, 20, "Spider", {"species:arachnid", "mob", "aggro", "auto"});
    ws.addEntity(30, 30, "Ralph", {"species:elf", "mob", "auto"});

    // Add command handlers for all entities.
    for (Entity& entity : ws.entities) {
        for (const Behavior::AbilitySet& abset : abilities) {
            std::vector<std::string> updated = abset.updateAvailable(entity);
        }
    }

    // Keep an easy handle to access the player
    // TODO If we keep this here is there any reason for the world state to bother tracking some
    // named entities?
    auto player_i = ws.findEntity(std::vector<std::string>{"player"});

    // Create a new window to display status.
    WINDOW* stat_window = newwin(40, 30, 0, ws.field_width + 10);

    // Create another window for the event log.
    WINDOW* event_window = newwin(40, 80, main_window_height, 0);
    std::deque<std::string> event_strings;

    std::vector<PANEL*> panels;
    panels.push_back(new_panel(event_window));
    panels.push_back(new_panel(stat_window));
    panels.push_back(new_panel(window));

    for (PANEL* panel : panels) {
        show_panel(panel);
        top_panel(panel);
    }

    // Create (and hide) a dialog window for help screens.
    std::map<std::string, UIComponent> help_components;
    {
        auto insert_stat = help_components.emplace(std::make_pair("abilities", UIComponent(ws, 38, 76, 1, 2)));
        UIComponent& uic = insert_stat.first->second;
        UserInterface::drawString(uic.window, "help", 0, 0);
        UserInterface::drawString(uic.window, "Type `help' and an ability name for more information.", 2, 0);
        UserInterface::drawString(uic.window, "Available abilities are:", 3, 0);
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
    dialog_box.renderDialogue(UserInterface::getDialogue("introduction"));
    // Don't block when checking for mouse clicks in the dialog window.
    wtimeout(dialog_box.window, 0);

    // update_panels should be called before rendering to any of the panels.
    update_panels();

    // Turn cursor visibility to normal.
    curs_set(1);

    // Wait 50 ms for user input and then handle any of the background game stuff that should be
    // happening. Unless the user has disabled ticking.
    if (0.0 < tick_rate) {
        wtimeout(window, 50);
    }

    ws.initialize();

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

    // Draw the player's status in the window
    {
        size_t status_row = UserInterface::drawStatus(stat_window, *player_i, 3, 1);
        UserInterface::drawHotkeys(stat_window, status_row+2, function_shortcuts);
    }

    // Update the world state.
    ws.update();

    // Update panels, refresh the screen, and reset the cursor position
    UserInterface::updateDisplay(window, ws.entities);
    UserInterface::clearInput(window, ws.field_height, ws.field_width);
    doupdate();

    // Initialize time stuff for the ticks
    auto last_update = std::chrono::steady_clock::now();

    bool quit = false;
    // TODO So bloated! A variable per dialog box?
    bool has_command = false;
    bool in_dialog = false;
    decltype(help_components)::iterator help_displayed = help_components.end();
    std::string command = "";

    // The introduction should be displayed immediately.
    in_dialog = true;
    dialog_box.show();

    // update_panels should be called before rendering to any of the panels.
    update_panels();
    doupdate();

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
                else if (UserInterface::hasDialogue(command)) {
                    dialog_box.renderDialogue(UserInterface::getDialogue(command));
                    dialog_box.show();
                    in_dialog = true;
                }
            }
            else if (0 < command.size() and UserInterface::hasDialogue(command)) {
                dialog_box.renderDialogue(UserInterface::getDialogue(command));
                dialog_box.show();
                in_dialog = true;
            }
            else {
                // If the user is issuing commands then exit help or dialog mode.
                if (help_displayed != help_components.end()) {
                    help_displayed->second.hide();
                    help_displayed = help_components.end();
                }
                if (in_dialog) {
                    in_dialog = false;
                    dialog_box.hide();
                }
                // Queue up actions and take them at the action tick.
                // TODO Support user aliases. Expand user aliases in the command string.
                // If there are semicolons then split the command into multiple at the semicolon
                // characters
                std::regex semicolon_or_end("(;|$)");
                std::smatch matches;
                while (0 < command.size() and std::regex_search(command, matches, semicolon_or_end)) {
                    comham.enqueueTraitCommand({"player"}, matches.prefix().str());
                    // Try to process the rest of the command
                    command = matches.suffix().str();
                }
                if (0 < command.size()) {
                    comham.enqueueTraitCommand({"player"}, command);
                }
                // There is a command to process.
                has_command = true;
            }
            // Update panel ordering.
            update_panels();
            //Update the cursor and clear the input field
            UserInterface::clearInput(window, ws.field_height, ws.field_width);
            command = "";
        }
        else if (in_c != ERR) {
            // Since we are in noecho mode the character should be drawn.
            wechochar(window, in_c);
            command.push_back(in_c);
        }

        // Check for mouse events
        if (in_dialog) {
            int in_c = wgetch(dialog_box.window);
            if (in_c == KEY_MOUSE) {
                MEVENT mevent;
                // We are only listening to click events.
                if (getmouse(&mevent)) {
                    std::cerr<<"Mouse click at "<<mevent.y<<", "<<mevent.x<<'\n';
                    // See if the mouse click was in the dialog_box button area.
                    std::string button_name = dialog_box.getButton(mevent.y, mevent.x);
                    if (button_name != "") {
                        // TODO FIXME The ui component should have logic associated with the
                        // buttons, but for now just continue.
                        in_dialog = false;
                        dialog_box.hide();
                    }
                }
            }
        }

        auto cur_time = std::chrono::steady_clock::now();
        std::chrono::duration<double> time_diff = cur_time - last_update;
        if (not in_dialog and help_displayed == help_components.end() and ws.entities.end() != player_i ) {
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
                // Update the last updated time and the time diff since it is used in some later
                // logic.
                last_update = cur_time;
                time_diff = cur_time - last_update;
                // execute all commands every tick
                comham.executeCommands(ws);
                // Tick update
                ws.update();
                auto player_entity = ws.findEntity(std::vector<std::string>{"player"});
                if (player_entity != ws.entities.end()) {
                // Find the user visible events.
                std::vector<std::string> player_events = ws.getLocalEvents(player_entity->y, player_entity->x, player_entity->stats.value().detectionRange());
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
                    UserInterface::updateEvents(event_window, event_strings);
                    // Update the player's status in the window
                    size_t status_row = UserInterface::drawStatus(stat_window, *player_entity, 3, 1);
                    status_row = UserInterface::drawInfolog(stat_window, status_row + 2, ws.info_log);
                    UserInterface::drawHotkeys(stat_window, status_row+2, function_shortcuts);
                    update_panels();
                }
                else {
                    // TODO Should play the last events that the player could have seen, since they
                    // will probably include the player's death.
                }
            }
        }

        // Update the player in case they have died or the trait has transferred to a new entity.
        player_i = ws.findEntity(std::vector<std::string>{"player"});
        // See if the player has died.
        if (ws.entities.end() == player_i and not in_dialog) {
            dialog_box.renderDialogue(UserInterface::getDialogue("game over"));
            dialog_box.show();
            in_dialog = true;
            update_panels();
        }

        // Update panels, refresh the screen, and reset the cursor position

        // In some systems writing to the game panel overwrites the dialog, even though its panel
        // should be on top of the game window.
        if (not in_dialog) {
            // Draw background effects in the first half of the tic.
            if (time_diff.count() < tick_rate / 2) {
                std::cerr<<"updating with "<<ws.background_effects.size()<<" background effects.\n";
                for (auto& [location, color] : ws.background_effects) {
                    std::cerr<<"\tlocation "<<std::get<0>(location)<<", "<<std::get<1>(location)<<" is "<<color<<'\n';
                }
                UserInterface::updateDisplay(window, ws.entities, ws.background_effects);
            }
            else {
                // Clear things that don't persist
                ws.background_effects.clear();
                UserInterface::updateDisplay(window, ws.entities);
            }
        }
        // Need to redraw the command since we've just erased the window.
        UserInterface::clearInput(window, ws.field_height, ws.field_width);
        for (char c : command) {
            waddch(window, c);
        }
        // Refresh the screen
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
