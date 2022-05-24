# Heart of Olympos
Rogue-like for friends.


## Prerequisites
nlohmann-json C++ library
ncurses >= 5 (and the panel library that should come with it)
C++ compiler that supports the C++20 standard.

## TODOs

* Help command
* * Open a new panel and pause the game
* * General help
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
* Add skills for all of the basic sensors (listen, taste, look, touch/feel, smell)
* * Make multiple rooms
* * World state to world state communication at "doorways". Can also go from entity -> world state -> world state ->
entity when it passes from one room to another.
* Mouse stuff (click on ability to add text).
* * See `curs_mouse`
