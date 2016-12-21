// vim: ts=4:sw=4 expandtab
#include "State.h"

State::State(const std::string &race, const std::unordered_map<std::string, EntityBP*> &blueprints_) :
    time(0),
    currentSupply(0),
    resources{0, 50},
    currentMaxSupply(0),
    unitMap{},
    workerMap{},
    buildingMap{},
    resourceMap{},
    timestamp(0),
    blueprints(blueprints_),
    alreadyProduced{},
    buildActions{},
    muleActions{} {
        bool mainBuilding = false;
        bool workers = false;
        for (const auto bp : blueprints) {
            if (bp.second->getRace() == race) {
                auto building = dynamic_cast<const BuildingBP*>(bp.second);
                if (building != nullptr && !mainBuilding) {
                    building->newInstance(*this);
                    mainBuilding = true;
                }
                auto unit = dynamic_cast<const UnitBP*>(bp.second);
                if (unit != nullptr && !workers) {
                    for (size_t i = 0; i < 6; i++) {
                        unit->newInstance(*this);
                    }
                    workers = true;
                }
            }
        }

        if (race == "zerg") {
            auto larva = static_cast<const UnitBP*>(blueprints.at("larva"));
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

std::map<int,WorkerInst>& State::getWorkers(){
    return workerMap;
}
std::map<int,BuildingInst>& State::getBuildings(){
    return buildingMap;
}
std::map<int,UnitInst>& State::getUnits(){
    return unitMap;
}
std::map<int,ResourceInst>& State::getResources(){
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
