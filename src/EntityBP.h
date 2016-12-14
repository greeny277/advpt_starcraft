// vim: ts=4:sw=4 expandtab
#pragma once

#include <vector>
#include <string>
#include <sstream>
#include "Resources.h"
#include "Ability.h"

class EntityBP {
protected:
    std::string name;
    std::string race;

    int startEnergy;
    int maxEnergy;
    Resources costs;
    int buildTime;
    std::vector<Ability*> abilities;
    std::vector<std::string> requireOneOf; // we need one of the listed entities
    std::vector<std::string> producedByOneOf; // second required entity
    std::vector<std::string> morphedFrom; // third required entity. This entity is gone once this was built.
    int supplyProvided;

	// TODO add abilities for specific entities
    inline EntityBP(std::string data[12]) :
        name(data[0]),
        race(data[8]),
        startEnergy(std::stoi(data[6])),
        maxEnergy(std::stoi(data[7])),
        costs(Resources(std::stoi(data[1]), std::stoi(data[2]))),
        buildTime(std::stoi(data[3])),
        abilities(),
        requireOneOf(),
        producedByOneOf({data[10]}),
        morphedFrom({data[9]}),
        supplyProvided(std::stoi(data[5])) {

        std::stringstream requirementStream(data[11]);
        std::string req;
        while (std::getline(requirementStream, req, '/')) {
            requireOneOf.push_back(req);
        }
    }

public:
    inline int getStartEnergy() { return startEnergy; }
    inline int getMaxEnergy() { return maxEnergy; }
    inline Resources getCosts() { return costs; }
    inline int getBuildTime() { return buildTime; }
    inline std::vector<Ability*>& getAbilities() { return abilities; }
    inline std::vector<std::string>& getRequireOneOf() { return requireOneOf; }
    inline std::vector<std::string>& getProducedByOneOf() { return producedByOneOf; }
    inline std::vector<std::string>& getMorphedFrom() { return morphedFrom; }
    inline int getSupplyProvided() { return supplyProvided; }
};

class UnitBP : public EntityBP {
    int supplyCost;

public:
    inline UnitBP(std::string data[12]) :
        EntityBP(data),
        supplyCost(std::stoi(data[4])) {

    }

    inline int getSupplyCost() { return supplyCost; }
};
class BuildingBP : public EntityBP {
public:
    inline BuildingBP(std::string data[13]) : EntityBP(data) {
    }
};
