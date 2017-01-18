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
    std::unordered_map<int,WorkerInst> workerMap;
    std::unordered_map<int,BuildingInst> buildingMap;
    std::unordered_map<int,UnitInst> unitMap;
    std::unordered_map<int,ResourceInst> resourceMap;
public:
    int time;
    Resources resources;
    std::vector<MuleAction> muleActions;
    std::vector<ChronoAction> chronoActions;
    std::vector<InjectAction> injectActions;
    std::vector<BuildEntityAction> buildActions;
    const std::unordered_map<std::string, std::unique_ptr<EntityBP>> &blueprints;
    std::unordered_multiset<std::string> alreadyProduced; // Keeps track of currently existing entities by name

public:
    State(const std::string &race, const std::unordered_map<std::string, std::unique_ptr<EntityBP>> &blueprints);
    nlohmann::json getUnitJSON();
    std::unordered_map<int, WorkerInst>& getWorkers();
    std::unordered_map<int, BuildingInst>& getBuildings();
    std::unordered_map<int, UnitInst>& getUnits();
    std::unordered_map<int, ResourceInst>& getResources();
    const std::unordered_map<int, WorkerInst>& getWorkers() const;
    const std::unordered_map<int, BuildingInst>& getBuildings() const;
    const std::unordered_map<int, UnitInst>& getUnits() const;
    const std::unordered_map<int, ResourceInst>& getResources() const;
    void addWorkerInst(WorkerInst);
    void addBuildingInst(BuildingInst);
    void addUnitInst(UnitInst);
    void addResourceInst(ResourceInst);
    void iterEntities(std::function<void(EntityInst&)>);
    void iterEntities(std::function<void(const EntityInst&)>) const;
    void eraseEntity(int id);
    EntityInst *getEntity(int id);
    void moveEntity(int old_id, int new_id);

    int computeUsedSupply() const;
    int computeMaxSupply() const;
};
