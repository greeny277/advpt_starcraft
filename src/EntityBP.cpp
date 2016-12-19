// vim: ts=4:sw=4 expandtab
#include "EntityBP.h"
#include "Ability.h"
#include "EntityInst.h"
#include <sstream>

static std::vector<std::string> parseRequirements(const std::string &requirements) {
    std::vector<std::string> requireOneOf;
    std::stringstream requirementStream(requirements);
    std::string req;
    while (std::getline(requirementStream, req, '/')) {
        requireOneOf.push_back(req);
    }
    return requireOneOf;
}
static std::vector<Ability*> createAbilities(const std::string &name) {
    std::vector<Ability*> abilities;
    if (name == "orbital_command") {
        abilities.push_back(new MuleAbility());
    }
    // TODO inject, chronoboost
    return abilities;
}

EntityBP::EntityBP(std::string data[15]) :
    name(data[0]),
    race(data[8]),
    startEnergy(std::stoi(data[6])),
    maxEnergy(std::stoi(data[7])),
    costs(Resources(std::stoi(data[1]), std::stoi(data[2]))),
    buildTime(std::stoi(data[3])),
    abilities(createAbilities(name)),
    requireOneOf(parseRequirements(data[11])),
    producedByOneOf({data[10]}),
    morphedFrom({data[9]}),
    supplyProvided(std::stoi(data[5])) {
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

UnitBP::UnitBP(std::string data[15]) :
    EntityBP(data),
    supplyCost(std::stoi(data[4])),
    isWorker(std::stoi(data[12])==1) {
}
int UnitBP::getSupplyCost() { return supplyCost; }
EntityInst *UnitBP::newInstance() const {
    if (isWorker) {
        return new WorkerInst(this);
    } else {
        return new UnitInst(this);
    }
}


BuildingBP::BuildingBP(std::string data[15]) :
    EntityBP(data),
    startResources(std::stoi(data[13]), std::stoi(data[14])) {
}
EntityInst *BuildingBP::newInstance() const {
    if (startResources.getGas() || startResources.getMinerals()) {
        return new ResourceInst(this);
    } else {
        return new BuildingInst(this);
    }
}
