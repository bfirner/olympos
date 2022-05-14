# Heart of Olympos
Rogue-like for friends.


## Prerequisites
nlohmann-json C++ library
ncurses (and the panel library that should come with it)
C++ compiler that supports the C++20 standard.

## TODOs

* Instead of using pointers for things, just keep all entities in the list and scan it for
  everything. Keep performance reasonable by limiting interactions from entity to entity within a
single room (world state). Communication can also go from entity -> world state -> world state ->
entity when it passes from one room to another.
* Handle entity collisions
* Support attacks and mob removal
* Support function key shortcuts
* Show commands in side bar
* Calculate vitality consumption based upon actions in a tick
* Add in status bars for vitality, hp, mana
