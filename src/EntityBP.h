// vim: ts=4:sw=4 expandtab
#pragma once

#include "Resources.h"
#include <string>
#include <vector>

class Ability;
class EntityInst;

class EntityBP {
    protected:
        const std::string name;
        const std::string race;

        const int startEnergy;
        const int maxEnergy;
        const Resources costs;
        const int buildTime; // TODO: buildTime=0 means the entity cannot be built (in a normal way)
        const std::vector<Ability*> abilities;
        const std::vector<std::string> requireOneOf; // we need one of the listed entities
        const std::vector<std::string> producedByOneOf; // second required entity
        const std::vector<std::string> morphedFrom; // third required entity. This entity is gone once this was built.
        const int supplyProvided;

        // TODO add abilities for specific entities
        EntityBP(std::string data[15]);

    public:
        const std::string & getName() const;
        const std::string & getRace() const;
        int getStartEnergy() const;
        int getMaxEnergy() const;
        Resources getCosts() const;
        int getBuildTime() const;
        const std::vector<Ability*>& getAbilities() const;
        const std::vector<std::string>& getRequireOneOf() const;
        const std::vector<std::string>& getProducedByOneOf() const;
        const std::vector<std::string>& getMorphedFrom() const;
        int getSupplyProvided() const;
        virtual EntityInst *newInstance() const = 0;
};

class UnitBP : public EntityBP {
    private:
        const int supplyCost;
        const bool isWorker;

    public:
        UnitBP(std::string data[15]);
        int getSupplyCost();
        EntityInst *newInstance() const;
};

class BuildingBP : public EntityBP {
    private:
        const Resources startResources;
    public:
        BuildingBP(std::string data[15]);
        EntityInst *newInstance() const;
};
