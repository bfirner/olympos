/*
 * Copyright 2022 Bernhard Firner
 *
 * Entity in the game. Has a position, a name, and some traits.
 */

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <string>

#include "entity.hpp"
#include "lore.hpp"
#include "olympos_utility.hpp"

using json = nlohmann::json;

// Get equipment slot information
json _equipment_slots;
json& getSlotInformation() {
    // Read in the file if it hasn't already been done.
    const std::filesystem::path equipment_slot_path{"resources/equipment_slots.json"};
    if (0 == _equipment_slots.size()) {
        if (std::filesystem::exists(equipment_slot_path)) {
            std::ifstream istream(equipment_slot_path.string(), std::ios::binary);
            if (istream) {
                std::string contents;
                std::getline(istream, contents, '\0');
                _equipment_slots = json::parse(contents);
            }
        }
    }
    return _equipment_slots;
}

// Initialize the class-wide variable.
std::atomic_size_t Entity::next_entity_id = 1;

size_t tickIncrease(double rate, size_t tick_num) {
    // Avoid storing any partial states by using the tick number to calculate if there are any whole
    // gains at this time step.
    // floor(rate * tick_num) - floor(rate * (tick_num-1))
    // floor(rate + remainder((rate * (tick_num-1), 1))
    // But those should only use the remainder if things were actually ticking up since last time.
    // Don't really want to store all of the past states of ticking or not though, should be fine.
    return floor(rate + fmod(rate * (tick_num-1), 1.0));
}

void Stats::ticHealthManaStamina(size_t tick_num) {
    double health_tick = vitality*0.1 + domain*0.05;
    double mana_tick = channel_rate * 0.1;
    double stamina_tick = 1.0 + cbrt(health_tick);

    // We obviously do not go beyond maximum values.
    health = std::min(maxHealth(), health + tickIncrease(health_tick, tick_num));
    mana = std::min(maxMana(), mana + tickIncrease(mana_tick, tick_num));
    stamina = std::min(maxStamina(), stamina + tickIncrease(stamina_tick, tick_num));
}

size_t Stats::maxMana() const {
    return std::floor(aura + domain);
}

size_t Stats::maxHealth() const {
    return std::floor(1.0 + vitality*0.8 + domain*0.2);
}

size_t Stats::maxStamina() const {
    // Control stamina growth to prevent insane turns later on.
    // Each point in stamina corresponds to roughly one action in a tick.
    return 1 + std::floor(cbrt(1.0 + vitality*0.5 + strength + domain*0.5));
}

size_t Stats::detectionRange() const {
    return 4 + std::floor(cbrt(vitality));
}

std::string Entity::getSpecies() const {
    auto species_location = std::find_if(traits.begin(), traits.end(),
            [](const std::string& entry){ return entry.starts_with("species:");});
    if (species_location == traits.end()) {
        return "";
    }
    return species_location->substr(std::string("species:").size());
}

std::string Entity::getObjectType() const {
    auto object_location = std::find_if(traits.begin(), traits.end(),
            [](const std::string& entry){ return entry.starts_with("object:");});
    if (object_location == traits.end()) {
        return "";
    }
    return object_location->substr(std::string("object:").size());
}

// Constructor
Entity::Entity(size_t y, size_t x, const std::string& name, const std::set<std::string> traits) {
    // Assign the entity ID and increment the classwide variable to ensure the ID remains unique.
    entity_id = Entity::next_entity_id.fetch_add(1);
    this->y = y;
    this->x = x;
    this->name = name;
    this->traits = traits;

    // If the traits defined a species then fill in stats. If there is no species then there are not
    // stats.
    stats = OlymposLore::getStats(*this);

    // Search the lore entries for either a species name or object type, depending upon what traits
    // this entity possesses.
    std::string search_key = getSpecies();
    if (0 == search_key.size()) {
        search_key = getObjectType();
    }

    // Get the character used to display this entity.
    std::string repr = OlymposLore::getLoreString(search_key, "character");
    // Fall back for objects without specific display characters.
    if (0 == repr.size()) {
        repr = ".";
    }
    character = OlymposUtility::utf8ToWString(repr);

    std::map<std::string, std::string> str_description =
        OlymposLore::getLoreData<std::map<std::string, std::string>>(search_key, "description");
    // Convert the strings to wstring and insert into this entity's description.
    for (auto [sense, str] : str_description) {
        description.insert(std::make_pair(sense, OlymposUtility::utf8ToWString(str)));
    }

    // Do this for all of [search_key, object]
    // Get the "is a" and "has a" relationships to expand traits.
    std::set<std::string> has_a = OlymposLore::getLoreField(search_key, "has a");
    this->traits.insert(has_a.begin(), has_a.end());

    // Get the traits of the groups of which this entity is a member.
    std::set<std::string> is_a = OlymposLore::getLoreField(search_key, "is a");
    for (const std::string& group : is_a) {
        this->traits.insert(group);
        std::set<std::string> has_a = OlymposLore::getLoreField(group, "has a");
        this->traits.insert(has_a.begin(), has_a.end());
    }

    behavior_set_name = OlymposLore::getLoreString(search_key, "base behavior");

    // TODO Load the behaviors granted by items here as well.

    // Go through all "has a" properties to determine equipment slots.
    json& slots = getSlotInformation();
    // Slot information is an array of slot names, what kinds of equipment they hold, and the
    // requirements to have that slot.
    // For example:
    //  {
    //      "name": "head",
    //      "requires": "head",
    //      "types": ["hat", "helmet"]
    //  },
    for (auto& slot_info : slots) {
        if (traits.contains(slot_info.at("requires").get<std::string>())) {
            // Remember that this slot is supported by this entity
            possible_slots.insert(slot_info.at("name").get<std::string>());
            // The "types" field will need to be checked to verify that a piece of equipment is
            // usable in a particular slot.
        }
    }
}

Entity::Entity(Entity&& other) : entity_id(other.entity_id), y(other.y), x(other.x), name(std::move(other.name)), traits(std::move(other.traits)), stats(other.stats), behavior_set_name(other.behavior_set_name), character(other.character), description(std::move(other.description)) {
    other.entity_id = 0;
}

std::string Entity::getDescription() const {
    return OlymposLore::getDescription(*this);
}

bool Entity::operator==(const Entity& other) const {
    return this->entity_id == other.entity_id;
}

bool Entity::operator==(const size_t other) const {
    return this->entity_id == other;
}
