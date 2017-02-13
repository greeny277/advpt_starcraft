// vim: ts=4:sw=4 expandtab
#include "State.h"
#include "EntityBP.h"
#include "Ability.h"
#include "EntityInst.h"
#include <sstream>

static std::vector<Ability*> createAbilities(const std::string &name) {
    std::vector<Ability*> abilities;
    if (name == "orbital_command") {
        static MuleAbility muleAbility;
        abilities.push_back(&muleAbility);
    } else if (name == "nexus") {
        static ChronoAbility chronoAbility;
        abilities.push_back(&chronoAbility);
    } else if (name == "queen") {
        static InjectAbility injectAbility;
        abilities.push_back(&injectAbility);
    }
    return abilities;
}

EntityBP::EntityBP(std::string data[15], bool is_unit_, bool is_worker_) :
    name(data[0]),
    race(raceFromString(data[8])),
    startEnergy(std::stoi(data[6])),
    maxEnergy(std::stoi(data[7])),
    costs(Resources(std::stoi(data[2]), std::stoi(data[1]))),
    buildTime(std::stoi(data[3])),
    abilities(createAbilities(name)),
    /*requireOneOf(parseRequirements(data[11], bps)),
    producedByOneOf(parseRequirements(data[10], bps)),
    morphedFrom(data[9].empty() ? std::vector<EntityBP*>{} : std::vector<EntityBP*>{bps.at(data[9]).get()}),*/
    requireOneOf(),
    producedByOneOf(),
    morphedFrom(),
    supplyProvided(std::stoi(data[5])),
    is_unit(is_unit_),
    is_worker(is_worker_) {
}

const std::string & EntityBP::getName() const { return name; }
Race EntityBP::getRace() const { return race; }
int EntityBP::getStartEnergy() const { return startEnergy; }
int EntityBP::getMaxEnergy() const { return maxEnergy; }
Resources EntityBP::getCosts() const { return costs; }
int EntityBP::getBuildTime() const { return buildTime; }
const std::vector<Ability*>& EntityBP::getAbilities() const { return abilities; }
std::vector<EntityBP*>& EntityBP::getRequireOneOf() { return requireOneOf; }
std::vector<EntityBP*>& EntityBP::getProducedByOneOf() { return producedByOneOf; }
std::vector<EntityBP*>& EntityBP::getMorphedFrom() { return morphedFrom; }
const std::vector<EntityBP*>& EntityBP::getRequireOneOf() const { return requireOneOf; }
const std::vector<EntityBP*>& EntityBP::getProducedByOneOf() const { return producedByOneOf; }
const std::vector<EntityBP*>& EntityBP::getMorphedFrom() const { return morphedFrom; }
int EntityBP::getSupplyProvided() const { return supplyProvided; }

UnitBP::UnitBP(std::string data[15]) :
    EntityBP(data, true, std::stoi(data[12])==1),
    supplyCost(std::stoi(data[4])) {
}
int UnitBP::getSupplyCost() const { return supplyCost; }
int UnitBP::newInstance(State &state) const {
    int id;

    if (is_worker) {
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
    EntityBP(data, false, false),
    buildSlots(name.rfind("_with_reactor") == std::string::npos ? 1 : 2),
    startResources(std::stoi(data[13]), std::stoi(data[14])) {
}
int BuildingBP::newInstance(State &state) const {
    int id;
    if (startResources.getGas() || startResources.getMinerals()) {
        auto newResource = ResourceInst(this);
        if(getName() == "hatchery"){
            for ( int i = 0; i < 3; ++i ) {
                newResource.createLarvae(state);
            }
        }
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
