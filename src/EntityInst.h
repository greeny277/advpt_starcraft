// vim: ts=4:sw=4 expandtab
#pragma once

#include "EntityBP.h"
#include "Action.h"

class EntityInst {
    EntityBP *blueprint;
    int currentEnergy;
    bool morphing; // when the unit is currently upgrading, the entity cannot do anything else
    virtual bool isBusy() = 0;
};
class UnitInst : public EntityInst {
    inline bool isBusy() { return false; }
};
class WorkerInst : public UnitInst {
    ResourceInst *workingResource;
    bool isBuilding;
    void assignToResource(ResourceInst &);
    BuildingStarted startBuilding(BuildingBP &);
    inline bool isBusy() { return isBuilding || workingResource != nullptr; }
};
class BuildingInst : public EntityInst {
    int freeBuildSlots;
    bool chronoBoostActivated;
};
class ResourceInst : public BuildingInst {
    Resources remaining;
    Resources miningRate;
    int maxWorkerSlots;
    int activeWorkerSlots;
    Resources mine();
    Resources getRemainingResources();
    void addWorker();
    void removeWorker();
};
