// vim: ts=4:sw=4 expandtab
#include "EntityBP.h"
#include <sstream>

EntityBP::EntityBP(std::string data[12]) :
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

const std::string & EntityBP::getName() const { return name; }
const std::string & EntityBP::getRace() const { return race; }
int EntityBP::getStartEnergy() const { return startEnergy; }
int EntityBP::getMaxEnergy() const { return maxEnergy; }
Resources EntityBP::getCosts() const { return costs; }
int EntityBP::getBuildTime() const { return buildTime; }
const std::vector<Ability*>& EntityBP::getAbilities() const { return abilities; }
const std::vector<std::string>& EntityBP::getRequireOneOf() const { return requireOneOf; }
const std::vector<std::string>& EntityBP::getProducedByOneOf() const { return producedByOneOf; }
const std::vector<std::string>& EntityBP::getMorphedFrom() const { return morphedFrom; }
int EntityBP::getSupplyProvided() const { return supplyProvided; }

UnitBP::UnitBP(std::string data[12]) :
    EntityBP(data),
    supplyCost(std::stoi(data[4])) {

    }
int UnitBP::getSupplyCost() { return supplyCost; }
BuildingBP::BuildingBP(std::string data[13]) : EntityBP(data) {
}
