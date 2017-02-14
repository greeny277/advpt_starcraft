// vim: ts=4:sw=4 expandtab
#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>
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
    std::unordered_multiset<const EntityBP*> alreadyProduced; // Keeps track of currently existing entities by name

    int usedSupply;
    int maxSupply;
    EntityBP *mainBuilding;
    int mainBuildingCount;

    EntityBP *gasBuilding;
    int gasBuildingCount;

public:
    State(const Race race, const std::unordered_map<std::string, std::unique_ptr<EntityBP>> &blueprints);
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
    template <typename T>
    void iterEntities(T);
    template <typename T>
    void iterEntities(T) const;
    void eraseEntity(int id);
    EntityInst *getEntity(int id);
    void moveEntity(int old_id, int new_id);

    inline int computeUsedSupply() const { return usedSupply; };
    inline int computeMaxSupply() const { return std::min(2000, maxSupply); };
    void adjustSupply(const EntityBP*, bool starting);
};
template <typename T>
inline void State::iterEntities(T f) {
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
template <typename T>
inline void State::iterEntities(T f) const {
    auto callback = [&] (EntityInst &ent) { f(const_cast<EntityInst&>(ent)); };
    const_cast<State*>(this)->iterEntities(callback);
}
