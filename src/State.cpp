// vim: ts=4:sw=4 expandtab
//
#include "State.h"

State::State(const Race race, const std::unordered_map<std::string, std::unique_ptr<EntityBP>> &blueprints_) :
    workerMap{},
    buildingMap{},
    unitMap{},
    resourceMap{},
    time(0),
    resources{0, 50},
    muleActions{},
    chronoActions{},
    injectActions{},
    buildActions{},
    blueprints(blueprints_),
    alreadyProduced{},
    usedSupply(0),
    maxSupply(0),
    mainBuildingCount(1),
    gasBuildingCount(0) {

        const char *workerName = nullptr;
        int mainBuildingID;

        if(race == PROTOSS) {
            workerName = "probe";
            mainBuildingID = blueprints.at("nexus").get()->newInstance(*this);
            mainBuilding = blueprints.at("nexus").get();
            gasBuilding = blueprints.at("assimilator").get();
        } else if(race == TERRAN) {
            workerName = "scv";
            mainBuildingID = blueprints.at("command_center")->newInstance(*this);
            mainBuilding = blueprints.at("command_center").get();
            gasBuilding = blueprints.at("refinery").get();
        } else if (race == ZERG) {
            workerName = "drone";
            mainBuildingID = blueprints.at("hatchery").get()->newInstance(*this);
            EntityBP *overlord = blueprints.at("overlord").get();
            overlord->newInstance(*this);
            adjustSupply(overlord, false);
            mainBuilding = blueprints.at("hatchery").get();
            gasBuilding = blueprints.at("extractor").get();
        } else {
            assert(false);
        }
        ResourceInst &mainBuildingRes = getResources().at(mainBuildingID);
        adjustSupply(mainBuildingRes.getBlueprint(), false);
        const UnitBP *worker = static_cast<const UnitBP*>(blueprints.at(workerName).get());
        for (size_t i = 0; i < 6; i++) {
            int workerID = worker->newInstance(*this);
            getWorkers().at(workerID).assignToResource(mainBuildingRes, *this);
            adjustSupply(worker, true);
        }
}
nlohmann::json State::getUnitJSON(){
    nlohmann::json units;

    iterEntities([&](EntityInst& inst){
        auto name = inst.getBlueprint()->getName();
        if (units.find(name) == std::end(units)) {
            units[name] = nlohmann::json::array();
        }
        units[name].push_back(std::to_string(inst.getID()));
    });
    return units;
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
    alreadyProduced.insert(e.getBlueprint());
    buildingMap.insert({e.getID(),e});
}
void State::addWorkerInst(WorkerInst e) {
    alreadyProduced.insert(e.getBlueprint());
    workerMap.insert({e.getID(),e});
}
void State::addUnitInst(UnitInst e) {
    alreadyProduced.insert(e.getBlueprint());
    unitMap.insert({e.getID(),e});
}
void State::addResourceInst(ResourceInst e) {
    alreadyProduced.insert(e.getBlueprint());
    resourceMap.insert({e.getID(), e});
}
void State::eraseEntity(int id) {
    alreadyProduced.erase(alreadyProduced.find(getEntity(id)->getBlueprint()));
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
void State::adjustSupply(const EntityBP *entity, bool starting) {
    if (starting) {
        if (!entity->getMorphedFrom().empty()) {
            auto front = entity->getMorphedFrom().front();
            if (front->is_unit) {
                usedSupply -= static_cast<const UnitBP*>(front)->getSupplyCost();
            }
        }
        if (entity->is_unit) {
            auto unit = static_cast<const UnitBP*>(entity);
            usedSupply += unit->getSupplyCost();
        }
    } else {
        maxSupply += entity->getSupplyProvided();

        if (!entity->getMorphedFrom().empty()) {
            auto front = entity->getMorphedFrom().front();
            maxSupply -= front->getSupplyProvided();
        }
    }
}

void State::moveEntity(int old_id, int new_id) {
    auto bent = buildingMap.find(old_id);
    if (bent != buildingMap.end()) {
        buildingMap.insert({new_id, bent->second}).first->second.setID(new_id);
        buildingMap.erase(bent);
    }
    auto went = workerMap.find(old_id);
    if (went != workerMap.end()) {
        workerMap.insert({new_id, went->second}).first->second.setID(new_id);
        workerMap.erase(went);
    }
    auto uent = unitMap.find(old_id);
    if (uent != unitMap.end()) {
        unitMap.insert({new_id, uent->second}).first->second.setID(new_id);
        unitMap.erase(uent);
    }
    auto rent = resourceMap.find(old_id);
    if (rent != resourceMap.end()) {
        resourceMap.insert({new_id, rent->second}).first->second.setID(new_id);
        resourceMap.erase(rent);
    }
}
