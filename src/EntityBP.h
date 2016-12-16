// vim: ts=4:sw=4 expandtab
#pragma once

#include "Resources.h"
#include "Ability.h"
#include <string>
#include <vector>

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
    EntityBP(std::string data[12]);

public:
     virtual ~EntityBP();
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
};

class UnitBP : public EntityBP {
    int supplyCost;

public:
     UnitBP(std::string data[12]);
    int getSupplyCost();
};

class BuildingBP : public EntityBP {
public:
     BuildingBP(std::string data[13]);
};
