# Heart of Olympos
Rogue-like for friends.


## Prerequisites
nlohmann-json C++ library
ncurses >= 5 (and the panel library that should come with it)
C++ compiler that supports the C++20 standard.

## TODOs

### Event logging
* Should the word "You" or "you" be highlighted in a color in the log?
* Movement failures not being logged.

### User Interface
* Add visual effects
* * Some kind of motion indicator?
* * Some kind of attack indicator?
* * Thrown objects should show up.
* * * These all imply animations that occur during "sub ticks"
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

### Code Cleanup
* Compartmentalize activities in main into separate functions
* Redo all "OR" options in the json files to be arrays of arrays.

### New Features
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
* Prototype the levelling process
* * Essence gains (experience) from combat and actions.
* * Skill gains
* * * Level from usage
* * * Gain new skills by actions in event messages (e.g. throw something on fire to build xp towards
      firebolt)
* * Class gains
* * * From associated skill usage
* * Skills exist under the umbrellas of classes
* * * Using a skill gives both the skill and the class experience
* * * The class also gains experience by hitting keywords for the class
* * * Skills advance by levels, by merging with other skills, by morphing laterally into new skills,
    or by advancing to improve their effectiveness
* * * * Levelling means a straight increase in current skill attributes
* * * * Merging means a new single skill that does both things with some loss of distant parts
* * * * * TODO Examples
* * * * Morphing means changing to a similar skill at the same level that has new abilities
* * * * * TODO Examples
* * * * Advancing means a straight increase to skill effects at an additional cost
* * * * * TODO Examples
* Elements and ability sets
* * Want to have ability sets with broad themes
* * Then adding an element into them makes them feel diverse and interesting
* * The entrypoint into the ability set should also have an impact
* * * (the entrypoint is the general skill(s) that allowed access to the ability set)
* Handle the player getting killed.
* * Restart after death.
* Multiple floors
* * Start with floor descriptions in json
* * Make multiple rooms per floor
* * World state to world state communication at "doorways". Can also go from entity -> world state -> world state ->
entity when it passes from one room to another.
* Mouse stuff (click on things to use ability, add hotkeys, etc)
* * See `curs_mouse`

