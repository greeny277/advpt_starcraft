// vim: ts=4:sw=4 expandtab

#include "EntityInst.h"

#include "Action.h"

int EntityInst::next_id = 0;

bool EntityInst::isBusy() const {
    return morphing;
}

 EntityInst::EntityInst(const EntityBP *bp) :
    blueprint(bp),
    id(next_id++) {
    }

 const EntityBP* EntityInst::getBlueprint() const { return blueprint; }

 const int EntityInst::getID() const { return id; }

 bool UnitInst::isBusy() { return false; }

 UnitInst::UnitInst(const UnitBP *unit) :
    EntityInst(unit) {
    }

 BuildingInst::BuildingInst(const BuildingBP *building) :
    EntityInst(building),
    freeBuildSlots(1),
    chronoBoostActivated(false) {
    }

 bool BuildingInst::isBusy() const {
    return freeBuildSlots == 0 || EntityInst::isBusy();
}

ResourceInst::ResourceInst(const BuildingBP *building) :
    BuildingInst(building),
    remaining(0, 0), // TODO: fix these values
    miningRate(0, 0),
    maxWorkerSlots(0),
    activeWorkerSlots(0) {
    }

 Resources ResourceInst::mine() {
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

bool ResourceInst::addWorker(){
    if(activeWorkerSlots == maxWorkerSlots){
        return false;
    } else {
        activeWorkerSlots++;
    }
    return true;
}

bool ResourceInst::removeWorker(){
    if(activeWorkerSlots == 0){
        return false;
    } else {
        activeWorkerSlots--;
    }
    return true;
}

 bool ResourceInst::getActiveWorkerCount() const { return activeWorkerSlots; }

 bool ResourceInst::isGas() const { return miningRate.gas > 0; }

 bool ResourceInst::isMinerals() const { return miningRate.minerals > 0; }

 WorkerInst::WorkerInst(const UnitBP *unit) :
    UnitInst(unit),
    workingResource(nullptr),
    isBuilding(false) {
    }
void WorkerInst::assignToResource(ResourceInst *r){
    workingResource = r;
}
 BuildingStarted *WorkerInst::startBuilding(BuildingBP *bbp, int curTime) {
    workingResource = nullptr;
    isBuilding = true;
    return new BuildingStarted(curTime, bbp, this);
}

 void WorkerInst::stopBuilding() {
    isBuilding = false;
    return;
}
 bool WorkerInst::isBusy() const { return isBuilding || workingResource != nullptr; }
