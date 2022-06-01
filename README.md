# Heart of Olympos
Rogue-like for friends.


## Prerequisites
nlohmann-json C++ library
ncurses >= 5 (and the panel library that should come with it)
C++ compiler that supports the C++20 standard.

## TODOs

* Handle the player getting killed.
* Aliases
* Change "dexterity" to "reflexes"
* Add visual effects
* * Color-code the event log
* * Some kind of motion indicator?
* * Some kind of attack indicator?
* Create behavior sets, which represent how entities should behave, in json
* * For example, define modes based upon proximity or visibility
* * * HP below some threshold: heal, flee
* * * Adjacent to player: attack
* * * Within x steps of player: chase
* * * Within x steps of player: kite
* * * Within x steps of player: ranged attacks
* * Define priority of different skills based upon the current mode
* Ability to hotkey commands
* Add skills for all of the basic sensors (listen, taste, look, touch/feel, smell)
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
