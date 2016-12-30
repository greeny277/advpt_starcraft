// vim: ts=4:sw=4 expandtab
#include "State.h"
#include "EntityBP.h"
#include "Ability.h"
#include "EntityInst.h"
#include <sstream>

static std::unordered_set<std::string> parseRequirements(const std::string &requirements) {
    std::unordered_set<std::string> requireOneOf;
    std::stringstream requirementStream(requirements);
    std::string req;
    while (std::getline(requirementStream, req, '/')) {
        if (!req.empty()) {
            requireOneOf.insert(req);
        }
    }
    return requireOneOf;
}
static std::vector<Ability*> createAbilities(const std::string &name) {
    std::vector<Ability*> abilities;
    if (name == "orbital_command") {
        static MuleAbility muleAbility;
        abilities.push_back(&muleAbility);
    }
    // TODO inject, chronoboost
    return abilities;
}

EntityBP::EntityBP(std::string data[15]) :
    name(data[0]),
    race(data[8]),
    startEnergy(std::stoi(data[6])),
    maxEnergy(std::stoi(data[7])),
    costs(Resources(std::stoi(data[2]), std::stoi(data[1]))),
    buildTime(std::stoi(data[3])),
    abilities(createAbilities(name)),
    requireOneOf(parseRequirements(data[11])),
    producedByOneOf(parseRequirements(data[10])),
    morphedFrom(data[9].empty() ? std::unordered_set<std::string>{} : std::unordered_set<std::string>{data[9]}),
    supplyProvided(std::stoi(data[5])) {
}

const std::string & EntityBP::getName() const { return name; }
const std::string & EntityBP::getRace() const { return race; }
int EntityBP::getStartEnergy() const { return startEnergy; }
int EntityBP::getMaxEnergy() const { return maxEnergy; }
Resources EntityBP::getCosts() const { return costs; }
int EntityBP::getBuildTime() const { return buildTime; }
const std::vector<Ability*>& EntityBP::getAbilities() const { return abilities; }
const std::unordered_set<std::string>& EntityBP::getRequireOneOf() const { return requireOneOf; }
const std::unordered_set<std::string>& EntityBP::getProducedByOneOf() const { return producedByOneOf; }
const std::unordered_set<std::string>& EntityBP::getMorphedFrom() const { return morphedFrom; }
int EntityBP::getSupplyProvided() const { return supplyProvided; }

UnitBP::UnitBP(std::string data[15]) :
    EntityBP(data),
    supplyCost(std::stoi(data[4])),
    isWorker(std::stoi(data[12])==1) {
}
int UnitBP::getSupplyCost() const { return supplyCost; }
int UnitBP::newInstance(State &state) const {
    int id;

    if (isWorker) {
        auto newWorker = WorkerInst(this);
        id = newWorker.getID();
        state.addWorkerInst(newWorker);
    } else {
        auto newUnit = UnitInst(this);
        id = newUnit.getID();
        state.addUnitInst(newUnit);
    }
    return id;
}


BuildingBP::BuildingBP(std::string data[15]) :
    EntityBP(data),
    buildSlots(name.rfind("_with_reactor") == std::string::npos ? 1 : 2),
    startResources(std::stoi(data[13]), std::stoi(data[14])) {
}
int BuildingBP::newInstance(State &state) const {
    int id;
    if (startResources.getGas() || startResources.getMinerals()) {
        auto newResource = ResourceInst(this);
        id = newResource.getID();
        state.addResourceInst(newResource);
    } else {
        auto newBuilding = BuildingInst(this);
        id = newBuilding.getID();
        state.addBuildingInst(newBuilding);
    }
    return id;
}
int BuildingBP::getBuildSlots() const { return buildSlots; }
