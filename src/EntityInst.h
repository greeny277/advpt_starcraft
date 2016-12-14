// vim: ts=4:sw=4 expandtab
#pragma once

#include "EntityBP.h"
#include "Action.h"

class EntityInst {
private:
    const EntityBP *blueprint;
    int currentEnergy;
    bool morphing; // when the unit is currently upgrading, the entity cannot do anything else
public:
    virtual bool isBusy() const {
        return morphing;
    }
    inline EntityInst(const EntityBP *bp) :
            blueprint(bp) {
    }
};
class UnitInst : public EntityInst {
public:
    inline bool isBusy() { return false; }
    inline UnitInst(const UnitBP *unit) :
            EntityInst(unit) {
    }
};
class BuildingInst : public EntityInst {
    int freeBuildSlots;
    bool chronoBoostActivated;
public:
    inline BuildingInst(const BuildingBP *building) :
            EntityInst(building),
            freeBuildSlots(1),
            chronoBoostActivated(false) {
    }
    inline bool isBusy() const {
        return freeBuildSlots == 0 || EntityInst::isBusy();
    }
};
class ResourceInst : public BuildingInst {
    Resources remaining;
    Resources miningRate;
    int maxWorkerSlots;
    int activeWorkerSlots;
public:
    inline Resources mine() {
        Resources out = miningRate * maxWorkerSlots;
        if (out.minerals > remaining.minerals)
            out.minerals = remaining.minerals;
        if (out.gas > remaining.gas)
            out.gas = remaining.gas;
        remaining = remaining - out;
        return out;
    }
    Resources getRemainingResources();
    void addWorker();
    void removeWorker();
};
class WorkerInst : public UnitInst {
    ResourceInst *workingResource;
    bool isBuilding;

public:
    inline WorkerInst(const UnitBP *unit) :
            UnitInst(unit),
            workingResource(nullptr),
            isBuilding(false) {
    }
    void assignToResource(ResourceInst &);
    BuildingStarted startBuilding(BuildingBP &);
    inline bool isBusy() { return isBuilding || workingResource != nullptr; }
};
