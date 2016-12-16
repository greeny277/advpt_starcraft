// vim: ts=4:sw=4 expandtab

#include "EntityInst.h"
#include "Action.h"

int EntityInst::next_id = 0;

bool EntityInst::isBusy() const {
    return morphing;
}

inline EntityInst::EntityInst(const EntityBP *bp) :
    blueprint(bp),
    id(next_id++) {
    }

inline const EntityBP* EntityInst::getBlueprint() { return blueprint; }

inline const int EntityInst::getID() const { return id; }

inline bool UnitInst::isBusy() { return false; }

inline UnitInst::UnitInst(const UnitBP *unit) :
    EntityInst(unit) {
    }

inline BuildingInst::BuildingInst(const BuildingBP *building) :
    EntityInst(building),
    freeBuildSlots(1),
    chronoBoostActivated(false) {
    }

inline bool BuildingInst::isBusy() const {
    return freeBuildSlots == 0 || EntityInst::isBusy();
}

ResourceInst::ResourceInst(const BuildingBP *building) :
    BuildingInst(building),
    remaining(0, 0), // TODO: fix these values
    miningRate(0, 0),
    maxWorkerSlots(0),
    activeWorkerSlots(0) {
    }

inline Resources ResourceInst::mine() {
    Resources out = miningRate * maxWorkerSlots;
    if (out.minerals > remaining.minerals)
        out.minerals = remaining.minerals;
    if (out.gas > remaining.gas)
        out.gas = remaining.gas;
    remaining = remaining - out;
    return out;
}

// TODO: Implement these three methods
Resources ResourceInst::getRemainingResources(){
    return remaining;
}

void ResourceInst::addWorker(){
    return;
}

void ResourceInst::removeWorker(){
    return;
}

inline bool ResourceInst::getActiveWorkerCount() const { return activeWorkerSlots; }

inline bool ResourceInst::isGas() const { return miningRate.gas > 0; }

inline bool ResourceInst::isMinerals() const { return miningRate.minerals > 0; }

inline WorkerInst::WorkerInst(const UnitBP *unit) :
    UnitInst(unit),
    workingResource(nullptr),
    isBuilding(false) {
    }
void WorkerInst::assignToResource(ResourceInst *r){
    workingResource = r;
}
inline BuildingStarted *WorkerInst::startBuilding(BuildingBP *bbp, int curTime) {
    workingResource = nullptr;
    isBuilding = true;
    return new BuildingStarted(curTime, bbp, this);
}

inline void WorkerInst::stopBuilding() {
    isBuilding = false;
    return;
}
inline bool WorkerInst::isBusy() const { return isBuilding || workingResource != nullptr; }
