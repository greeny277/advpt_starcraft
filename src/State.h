// vim: ts=4:sw=4 expandtab
#pragma once

#include <string>
#include <unordered_map>
#include <set>
#include "Resources.h"
#include "EntityBP.h"
#include "EntityInst.h"
#include "Action.h"
#include "json.hpp"


class State {
private:
    std::map<int,WorkerInst> workerMap;
    std::map<int,BuildingInst> buildingMap;
    std::map<int,UnitInst> unitMap;
    std::map<int,ResourceInst> resourceMap;
public:
    int time;
    int currentSupply;
    Resources resources;
    int currentMaxSupply;
    std::vector<MuleAction> muleActions;
    std::vector<BuildEntityAction> buildActions;
    const std::unordered_map<std::string, EntityBP*> &blueprints;
    std::set<std::string> alreadyProduced; // Keeps track of entities which were produced once at least

public:
    State(const std::string &race, const std::unordered_map<std::string, EntityBP*> &blueprints);
    nlohmann::json getUnitJSON();
    std::map<int, WorkerInst>& getWorkers();
    std::map<int, BuildingInst>& getBuildings();
    std::map<int, UnitInst>& getUnits();
    std::map<int, ResourceInst>& getResources();
    const std::map<int, WorkerInst>& getWorkers() const;
    const std::map<int, BuildingInst>& getBuildings() const;
    const std::map<int, UnitInst>& getUnits() const;
    const std::map<int, ResourceInst>& getResources() const;
    void addWorkerInst(WorkerInst);
    void addBuildingInst(BuildingInst);
    void addUnitInst(UnitInst);
    void addResourceInst(ResourceInst);
    void iterEntities(std::function<void(EntityInst&)>);
    void iterEntities(std::function<void(const EntityInst&)>) const;
    void eraseEntity(int id);
    EntityInst *getEntity(int id);
};
