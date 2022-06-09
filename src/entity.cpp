/*
 * Copyright 2022 Bernhard Firner
 *
 * Entity in the game. Has a position, a name, and some traits.
 */

#include <iostream>
#include <algorithm>
#include <cmath>
#include <set>
#include <string>

#include "entity.hpp"
#include "lore.hpp"
#include "olympos_utility.hpp"

// Initialize the class-wide variable.
std::atomic_size_t Entity::next_entity_id = 0;

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

// Constructor
Entity::Entity(size_t y, size_t x, const std::string& name, const std::set<std::string> traits) {
    // Assign the entity ID and increment the classwide variable to ensure the ID remains unique.
    entity_id = Entity::next_entity_id.fetch_add(1);
    std::cerr<<"Constructing an entity with id "<<entity_id<<'\n';
    std::cerr<<"Next entity id is "<<Entity::next_entity_id<<'\n';
    this->y = y;
    this->x = x;
    this->name = name;
    this->traits = traits;

    // If the traits defined a species then fill in stats. If there is no species then
    stats = OlymposLore::getStats(*this);

    std::string species = getSpecies();

    // Get the character used to display this creature.
    std::string repr = OlymposLore::getSpeciesString(species, "character");
    if (0 == repr.size()) {
        repr = ".";
    }
    character = OlymposUtility::utf8ToWString(repr);

    std::map<std::string, std::string> str_description =
        OlymposLore::getSpeciesData<std::map<std::string, std::string>>(species, "description");
    // Convert the strings to wstring and insert into this entity's description.
    for (auto [sense, str] : str_description) {
        description.insert(std::make_pair(sense, OlymposUtility::utf8ToWString(str)));
    }

    // Get the "is a" and "has a" relationships to expand traits.
    std::set<std::string> has_a = OlymposLore::getSpeciesField(species, "has a");
    this->traits.insert(has_a.begin(), has_a.end());

    // Get the traits of the groups of which this entity is a member.
    std::set<std::string> is_a = OlymposLore::getSpeciesField(species, "is a");
    for (const std::string& group : is_a) {
        this->traits.insert(group);
        std::set<std::string> has_a = OlymposLore::getSpeciesField(group, "has a");
        this->traits.insert(has_a.begin(), has_a.end());
    }

    behavior_set_name = OlymposLore::getSpeciesString(species, "base behavior");
}

Entity::Entity(const Entity& other) : entity_id(other.entity_id), y(other.y), x(other.x), name(other.name), traits(other.traits), stats(other.stats), behavior_set_name(other.behavior_set_name), character(other.character) {
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
