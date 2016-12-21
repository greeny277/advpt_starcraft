// vim: ts=4:sw=4 expandtab

#include "EntityInst.h"

#include "Action.h"
#include "State.h"

#include<set>

int EntityInst::next_id = 0;

bool EntityInst::isBusy() const {
    return morphing;
}

bool EntityInst::checkBuildRequirements(EntityBP *entity, State &s) {
    // check free slots
    if(isBusy()) {
        return false;
    }
    // check for resources
    if(!s.resources.allValuesLargerThan(entity->getCosts())) {
        // Not enough resources available
        return false;
    }
    // check if this building can produce the unit. Therefore get list of buildings which
    // can produce the entity and check if this building is one of them
    std::vector<std::string> buildingNames = entity->getProducedByOneOf();
    auto it = std::find(std::begin(buildingNames), std::end(buildingNames), getBlueprint()->getName());
    if(it == std::end(buildingNames)){
        // this building can not produce the unit
        return false;
    }

    // check requirements
    std::vector<std::string> requirementNames = entity->getRequireOneOf();
    bool foundRequirement = false;
    for(auto it = requirementNames.begin(); it != requirementNames.end(); it++){
        auto iterProduced = std::find(std::begin(s.alreadyProduced), std::end(s.alreadyProduced), *it);
        if(iterProduced != std::end(s.alreadyProduced)){
            foundRequirement = true;
            break;
        }
    }
    if(!foundRequirement)
        return false;

    // TODO: supply

    return true;
}
bool EntityInst::isMorphing() const {
    return morphing;
}

void EntityInst::setMorphing(bool b){
    morphing =  b;
}
bool EntityInst::startMorphing(EntityBP *entity, State &s) {
    if (!checkBuildRequirements(entity, s)) {
        return false;
    }
    bool foundBp = false;
    for (auto bp : entity->getMorphedFrom()) {
        if (blueprint->getName() == bp)
            foundBp = true;
    }
    if (!foundBp)
        return false;
    // TODO: if this is a building that can do more than one thing, check that we are really doing nothing right now

    s.buildActions.push_back(BuildEntityAction(s.time, entity, -1, getID(), s));
    return true;
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

/** This method starts the mechanism of producing an unit in the building. Therefore
 *  zergs are not covered yet.
 *
 *  @return: Returns either a @BuildEntityAction or
 *  a @nullptr when any requirment is not fulfilled
 *
 *  **/
bool BuildingInst::produceUnit(UnitBP *entity, State &s) {
    if (!checkBuildRequirements(entity, s) || !entity->getMorphedFrom().empty()) {
        return false;
    }

    freeBuildSlots--;

    s.buildActions.push_back(BuildEntityAction(s.time, entity, -1, getID(), s));
    return true;
}

void BuildingInst::incFreeBuildSlots(){
    freeBuildSlots++;
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
int ResourceInst::getFreeWorkerCount() const { return maxWorkerSlots - activeWorkerSlots; }

bool ResourceInst::isGas() const { return miningRate.getGas() > 0; }

bool ResourceInst::isMinerals() const { return miningRate.getMinerals() > 0; }

WorkerInst::WorkerInst(const UnitBP *unit) :
    UnitInst(unit),
    workingResource(-1),
    isBuilding(false) {
    }
void WorkerInst::assignToResource(ResourceInst& r){
    workingResource = r.getID();
}
BuildEntityAction *WorkerInst::startBuilding(BuildingBP *bbp, int curTime, State &s) {
    workingResource = -1;
    isBuilding = true;
    // TODO: check requirements
    return new BuildEntityAction(curTime, bbp, getID(), -1, s);
}

void WorkerInst::stopBuilding() {
    isBuilding = false;
    return;
}
bool WorkerInst::isBusy() const { return isBuilding || workingResource != -1; }
bool WorkerInst::isMiningGas(State &s) const {
    if (workingResource == -1)
        return false;

    return s.getResources().at(workingResource).isGas();
}
bool WorkerInst::isMiningMinerals(State &s) const {
    if (workingResource == -1)
        return false;

    return s.getResources().at(workingResource).isMinerals();
}
