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
    remaining(building->startResources),
    miningRate(0, 0),
    maxWorkerSlots(0),
    activeWorkerSlots(0) {
        if (building->startResources.getGas() > 0) {
            maxWorkerSlots = 3;
            miningRate.setMilliGas(350);
        } else {
            // TODO: fix remaining minerals for upgraded buildings
            maxWorkerSlots = 16;
            miningRate.setMilliMinerals(700);
        }
}

Resources ResourceInst::mine() {
    Resources out = miningRate * activeWorkerSlots;
    if (out.getMinerals() > remaining.getMinerals())
        out.setMinerals(remaining.getMinerals());
    if (out.getGas() > remaining.getGas())
        out.setGas(remaining.getGas());
    remaining = remaining - out;
    return out;
}

Resources ResourceInst::getRemainingResources() const {
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

int ResourceInst::getActiveWorkerCount() const { return activeWorkerSlots; }

bool ResourceInst::isGas() const { return miningRate.getGas() > 0; }

bool ResourceInst::isMinerals() const { return miningRate.getMinerals() > 0; }

WorkerInst::WorkerInst(const UnitBP *unit) :
    UnitInst(unit),
    workingResource(nullptr),
    isBuilding(false) {
    }
void WorkerInst::assignToResource(ResourceInst *r){
    workingResource = r;
}
BuildEntityAction *WorkerInst::startBuilding(BuildingBP *bbp, int curTime) {
    workingResource = nullptr;
    isBuilding = true;
    return new BuildEntityAction(curTime, bbp, this);
}

void WorkerInst::stopBuilding() {
    isBuilding = false;
    return;
}
bool WorkerInst::isBusy() const { return isBuilding || workingResource != nullptr; }
