# Heart of Olympos
Rogue-like for friends.


## Prerequisites
nlohmann-json C++ library
ncurses (and the panel library that should come with it)
C++ compiler that supports the C++20 standard.

## TODOs

* Improve event messages.
* Support function key shortcuts
* Show common and hotkey commands in side bar
* Help command
* * Details on available skills
* * Ability to hotkey commands
* Make more complicated mob behavior
* * Follow
* * Follow within some distance
* * Attack if able
* Multiple floors
* * Start with floor descriptions in json
* Prototype the following
* * Essence gains (experience) from combat and actions.
* * Skill gains
* * * Level from usage
* * * Gain new skills by actions in event messages (e.g. throw something on fire to build xp towards
      firebolt)
* * Class gains
* * * From associated skill usage
* * Make multiple rooms
* * World state to world state communication at "doorways". Can also go from entity -> world state -> world state ->
entity when it passes from one room to another.
