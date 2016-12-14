// vim: ts=4:sw=4 expandtab
#pragma once

#include "EntityBP.h"

class BuildingStarted;

class EntityInst {
private:
    const EntityBP *blueprint;
    int currentEnergy;
    bool morphing; // when the unit is currently upgrading, the entity cannot do anything else
    const int id;
    static int next_id;
public:
    virtual bool isBusy() const {
        return morphing;
    }
    inline EntityInst(const EntityBP *bp) :
            blueprint(bp),
            id(next_id++) {
    }
    inline const EntityBP *getBlueprint() const { return blueprint; }
    inline const int getID() const { return id; }
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
    ResourceInst(const BuildingBP *building) :
        BuildingInst(building),
        remaining(0, 0), // TODO: fix these values
        miningRate(0, 0),
        maxWorkerSlots(0),
        activeWorkerSlots(0) {
    }

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

    inline bool getActiveWorkerCount() const { return activeWorkerSlots; }
    inline bool isGas() const { return miningRate.gas > 0; }
    inline bool isMinerals() const { return miningRate.minerals > 0; }
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
    /*inline BuildingStarted startBuilding(BuildingBP &) {
        TODO
    }*/
    inline bool isBusy() const { return isBuilding || workingResource != nullptr; }
};
