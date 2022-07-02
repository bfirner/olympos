# Heart of Olympos
Rogue-like for friends.


## Prerequisites
nlohmann-json C++ library
ncurses >= 5 (and the panel library that should come with it)
C++ compiler that supports the C++20 standard.

## TODOs

* Killed entities are not removed from the map and prevent future motion through those tiles.
* Compartmentalize activities in main into separate functions
* Fix the (s) at the end of words when entity is replaced by "You" (e.g. "You looks", "You kicks", etc)
* * Should the word "You" or "you" be highlighted in a color?
* Utility and tool commands
* * Pick up
* * Throw
* * Swing
* * * This all implies having and wielding objects and an inventory
* Objects
* * Should have a list of actions that can be done with them
* * Actions should list things that are required to do them
* * E.g. to swing a sword an entity requires "hand" and "arm" or "pseudopod" or "tentacle"
* * * "requirements": [["hand", "arm"], ["pseudopod"], ["tentacle"]]
* * Take advantage of inheritence like with entities
* Redo all "OR" options in the json files to be arrays of arrays.
* Add visual effects
* * Some kind of motion indicator?
* * Some kind of attack indicator?
* * Thrown objects should show up.
* * * These all imply animations that occur during "sub ticks"
* Prototype the following
* * Essence gains (experience) from combat and actions.
* * Skill gains
* redo all "OR" options in the json files to be arrays of arrays.
* * * Level from usage
* * * Gain new skills by actions in event messages (e.g. throw something on fire to build xp towards
      firebolt)
* * Class gains
* * * From associated skill usage
* * Skills exist under the umbrellas of classes
* * * Using a skill gives both the skill and the class experience
* * * The class also gains experience by hitting keywords for the class
* * * Skills advance by levels, by merging with other skills, or by morphing into new skills
* * * * Levelling means a straight increase in current skill attributes
* * * * Merging means a new single skill that does both things with some loss of distant parts
* * * * * TODO Examples
* * * * Morphing means changing to a similar skill at the same level that has new abilities
* * * * * TODO Examples
* Add a tutorial
* * Go over the UI
* * Go over commands
* * Go over skills and classes
* * Demonstration of skill learning
* Help command has non-skill help
* * General help
* * UI help
* * status help
* Add more dialog handling
* * Plus animations in the dialog!
* Aliases
* Ability to hotkey commands
* Keep updating the "help" menu with game commands (e.g. alias and hotkey)
* Create category aliases for the behavior sets
* * Like attack, but for other stuff as well.
* * Or add more conditions to check if general abilities exist
* * Need compound logic as well I suppose
* Handle the player getting killed.
* * Restart after death.
* Multiple floors
* * Start with floor descriptions in json
* * Make multiple rooms per floor
* * World state to world state communication at "doorways". Can also go from entity -> world state -> world state ->
entity when it passes from one room to another.
* Mouse stuff (click on things to use ability, add hotkeys, etc)
* * See `curs_mouse`
* More complicated senses
* * Should work with things in inventory, or in general
* Each sense should have a list of traits that can be picked up.
* If an entity has those traits then it will trigger that sense
* * e.g. if Orcs have the "rotten" trait then "smell" could detect them.
* * e.g. "smell" -> "You smell the rotten stench of an Orc."

* Elements and ability sets
* * Want to have ability sets with broad themes
* * Then adding an element into them makes them feel diverse and interesting
* * The entrypoint into the ability set should also have an impact
* * * (the entrypoint is the general skill(s) that allowed access to the ability set)
