// vim: ts=4:sw=4 expandtab
#pragma once

#include "Resources.h"
#include <string>
#include <vector>
#include <unordered_set>

class Ability;
class EntityInst;
class State;

class EntityBP {
    protected:
        const std::string name;
        const std::string race;

        const int startEnergy;
        const int maxEnergy;
        const Resources costs;
        const int buildTime;
        const std::vector<Ability*> abilities;
        const std::unordered_set<std::string> requireOneOf; // we need one of the listed entities
        const std::unordered_set<std::string> producedByOneOf; // second required entity
        const std::unordered_set<std::string> morphedFrom; // third required entity. This entity is gone once this was built.
        const int supplyProvided;

        explicit EntityBP(std::string data[15]);

    public:
        const std::string & getName() const;
        const std::string & getRace() const;
        int getStartEnergy() const;
        int getMaxEnergy() const;
        Resources getCosts() const;
        int getBuildTime() const;
        const std::vector<Ability*>& getAbilities() const;
        const std::unordered_set<std::string>& getRequireOneOf() const;
        const std::unordered_set<std::string>& getProducedByOneOf() const;
        const std::unordered_set<std::string>& getMorphedFrom() const;
        int getSupplyProvided() const;
        virtual int newInstance(State&) const = 0;
        virtual ~EntityBP() = default;
};

class UnitBP : public EntityBP {
    private:
        const int supplyCost;
        const bool isWorker;

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
