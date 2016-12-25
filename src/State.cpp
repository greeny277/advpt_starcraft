// vim: ts=4:sw=4 expandtab
#include "State.h"

State::State(const std::string &race, const std::unordered_map<std::string, std::unique_ptr<EntityBP>> &blueprints_) :
    workerMap{},
    buildingMap{},
    unitMap{},
    resourceMap{},
    time(0),
    resources{0, 50},
    muleActions{},
    buildActions{},
    blueprints(blueprints_),
    alreadyProduced{} {

        if(race == "protoss") {
            auto probe = static_cast<const UnitBP*>(blueprints.at("probe").get());
            for (size_t i = 0; i < 6; i++) {
                probe->newInstance(*this);
            }
            blueprints.at("nexus").get()->newInstance(*this);
        }

        if(race == "terran") {
            auto scv = static_cast<const UnitBP*>(blueprints.at("scv").get());
            for (size_t i = 0; i < 6; i++) {
                scv->newInstance(*this);
            }
            blueprints.at("command_center")->newInstance(*this);
        }

        if (race == "zerg") {
            auto drone = static_cast<const UnitBP*>(blueprints.at("drone").get());
            for (size_t i = 0; i < 6; i++) {
                drone->newInstance(*this);
            }
            blueprints.at("hatchery").get()->newInstance(*this);

            auto larva = static_cast<const UnitBP*>(blueprints.at("larva").get());
            for (size_t i = 0; i < 3; i++) {
                larva->newInstance(*this);
            }
            blueprints.at("overlord")->newInstance(*this);
        }
}
nlohmann::json State::getUnitJSON(){
    nlohmann::json units;

    iterEntities([&](EntityInst& inst){
        auto name = inst.getBlueprint()->getName();
        if (units.find(name) == std::end(units)) {
            units[name] = nlohmann::json::array();
        }
        units[name].push_back(inst.getID());
    });
    return units;
}
void State::iterEntities(std::function<void(EntityInst&)> f) {
    for(auto& i : workerMap) {
        f(i.second);
    }
    for(auto& i : unitMap) {
        f(i.second);
    }
    for(auto& i : buildingMap) {
        f(i.second);
    }
    for(auto& i : resourceMap) {
        f(i.second);
    }
}
void State::iterEntities(std::function<void(const EntityInst&)> f) const {
    auto callback = [&] (EntityInst &ent) { f(const_cast<EntityInst&>(ent)); };
    const_cast<State*>(this)->iterEntities(callback);
}

std::unordered_map<int,WorkerInst>& State::getWorkers(){
    return workerMap;
}
std::unordered_map<int,BuildingInst>& State::getBuildings(){
    return buildingMap;
}
std::unordered_map<int,UnitInst>& State::getUnits(){
    return unitMap;
}
std::unordered_map<int,ResourceInst>& State::getResources(){
    return resourceMap;
}
const std::unordered_map<int,WorkerInst>& State::getWorkers() const {
    return workerMap;
}
const std::unordered_map<int,BuildingInst>& State::getBuildings() const {
    return buildingMap;
}
const std::unordered_map<int,UnitInst>& State::getUnits() const {
    return unitMap;
}
const std::unordered_map<int,ResourceInst>& State::getResources() const {
    return resourceMap;
}

void State::addBuildingInst(BuildingInst e) {
    alreadyProduced.insert(e.getBlueprint()->getName());
    buildingMap.insert({e.getID(),e});
}
void State::addWorkerInst(WorkerInst e) {
    alreadyProduced.insert(e.getBlueprint()->getName());
    workerMap.insert({e.getID(),e});
}
void State::addUnitInst(UnitInst e) {
    alreadyProduced.insert(e.getBlueprint()->getName());
    unitMap.insert({e.getID(),e});
}
void State::addResourceInst(ResourceInst e) {
    alreadyProduced.insert(e.getBlueprint()->getName());
    resourceMap.insert({e.getID(), e});
}
void State::eraseEntity(int id) {
    buildingMap.erase(id);
    workerMap.erase(id);
    unitMap.erase(id);
    resourceMap.erase(id);
}
EntityInst *State::getEntity(int id) {
    auto bent = buildingMap.find(id);
    if (bent != buildingMap.end())
        return &bent->second;
    auto went = workerMap.find(id);
    if (went != workerMap.end())
        return &went->second;
    auto uent = unitMap.find(id);
    if (uent != unitMap.end())
        return &uent->second;
    auto rent = resourceMap.find(id);
    if (rent != resourceMap.end())
        return &rent->second;
    return nullptr;
}
int State::computeUsedSupply() const {
    int supply = 0;
    // compute supply of existing units (skip morphing units)
    iterEntities([&](const EntityInst &e) {
        if (!e.isMorphing()) {
            if (auto bp = dynamic_cast<const UnitBP*>(e.getBlueprint())) {
                supply += bp->getSupplyCost();
            }
        }
    });
    // compute supply of units that are getting built right now
    for (const auto &action : buildActions) {
        if (action.hasFinished()) // skip finished actions to avoid double counting
            continue;

        if (auto bp = dynamic_cast<const UnitBP*>(action.getBlueprint())) {
            supply += bp->getSupplyCost();
        }
    }

    return supply;
}
int State::computeMaxSupply() const {
    int supply = 0;
    iterEntities([&](const EntityInst &e) {
        supply += e.getBlueprint()->getSupplyProvided();
    });
    return supply;
}
