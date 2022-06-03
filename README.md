# Heart of Olympos
Rogue-like for friends.


## Prerequisites
nlohmann-json C++ library
ncurses >= 5 (and the panel library that should come with it)
C++ compiler that supports the C++20 standard.

## TODOs

* help topics
* * help actions
* * help abilities
* * help stats, statistics, attributes
* Add more dialog handling
* * Plus animations in the dialog!
* Aliases
* Ability to hotkey commands
* Keep updating the "help" menu with game commands (e.g. alias and hotkey)
* Add visual effects
* * Color-code the event log
* * Some kind of motion indicator?
* * Some kind of attack indicator?
* Create category aliases for the behavior sets
* * Like attack, but for other stuff as well.
* * Or add more conditions to check if general abilities exist
* * Need compound logic as well I suppose
* Add skills for all of the basic senses (listen, taste, look, touch/feel, smell)
* Handle the player getting killed.
* * Restart after death.
* Utility and tool commands
* * Pick up
* * Throw
* * Swing
* Prototype the following
* * Essence gains (experience) from combat and actions.
* * Skill gains
* * * Level from usage
* * * Gain new skills by actions in event messages (e.g. throw something on fire to build xp towards
      firebolt)
* * Class gains
* * * From associated skill usage
* Help command has non-skill help
* * General help
* * UI help
* Multiple floors
* * Start with floor descriptions in json
* * Make multiple rooms per floor
* * World state to world state communication at "doorways". Can also go from entity -> world state -> world state ->
entity when it passes from one room to another.
* Mouse stuff (click on things to use ability, add hotkeys, etc)
* * See `curs_mouse`
