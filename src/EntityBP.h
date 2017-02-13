// vim: ts=4:sw=4 expandtab
#pragma once

#include "Resources.h"
#include <string>
#include <vector>
#include <cassert>
#include <unordered_map>
#include <memory>

class Ability;
class EntityInst;
class State;

enum Race {
    ZERG,
    PROTOSS,
    TERRAN,
};
inline enum Race raceFromString(const std::string &str) {
    if (str == "zerg")
        return ZERG;
    else if (str == "protoss")
        return PROTOSS;
    assert(str == "terran");
    return TERRAN;
}
inline std::string raceToString(const Race race) {
    switch(race) {
        case ZERG:
            return "zerg";
        case PROTOSS:
            return "protoss";
        case TERRAN:
            return "terran";
    }
    assert(false && "invalid race");
}

class EntityBP {
    protected:
        const std::string name;
        const Race race;

        const int startEnergy;
        const int maxEnergy;
        const Resources costs;
        const int buildTime;
        const std::vector<Ability*> abilities;
        std::vector<EntityBP*> requireOneOf; // we need one of the listed entities
        std::vector<EntityBP*> producedByOneOf; // second required entity
        std::vector<EntityBP*> morphedFrom; // third required entity. This entity is gone once this was built.
        const int supplyProvided;

        explicit EntityBP(std::string data[15], bool, bool);
    public:
        const bool is_unit;
        const bool is_worker;

        const std::string & getName() const;
        Race getRace() const;
        int getStartEnergy() const;
        int getMaxEnergy() const;
        Resources getCosts() const;
        int getBuildTime() const;
        const std::vector<Ability*>& getAbilities() const;
        std::vector<EntityBP*>& getRequireOneOf();
        std::vector<EntityBP*>& getProducedByOneOf();
        std::vector<EntityBP*>& getMorphedFrom();
        const std::vector<EntityBP*>& getRequireOneOf() const;
        const std::vector<EntityBP*>& getProducedByOneOf() const;
        const std::vector<EntityBP*>& getMorphedFrom() const;
        int getSupplyProvided() const;
        virtual int newInstance(State&) const = 0;
        virtual ~EntityBP() = default;
};

class UnitBP : public EntityBP {
    private:
        const int supplyCost;

    public:
        explicit UnitBP(std::string data[15]);
        int getSupplyCost() const;
        int newInstance(State&) const override;
};

class BuildingBP : public EntityBP {
    private:
        const int buildSlots;
    public:
        const Resources startResources;
        explicit BuildingBP(std::string data[15]);
        int newInstance(State&) const override;
        int getBuildSlots() const;

};
