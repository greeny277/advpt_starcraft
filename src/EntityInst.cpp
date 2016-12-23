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
    if(!s.resources.allValuesLargerEquals(entity->getCosts())) {
        // Not enough resources available
        return false;
    }
    // check if this building can produce the unit. Therefore get list of buildings which
    // can produce the entity and check if this building is one of them
    const std::vector<std::string> &buildingNames = entity->getProducedByOneOf();
    auto it = std::find(std::begin(buildingNames), std::end(buildingNames), getBlueprint()->getName());
    if(it == std::end(buildingNames)){
        // this building can not produce the unit
        return false;
    }

    // check requirements
    const std::vector<std::string> &requirementNames = entity->getRequireOneOf();
    bool foundRequirement = false;
    for (auto &req : requirementNames) {
        auto iterProduced = std::find(std::begin(s.alreadyProduced), std::end(s.alreadyProduced), req);
        if(iterProduced != std::end(s.alreadyProduced)){
            foundRequirement = true;
            break;
        }
    }
    if(!foundRequirement && !requirementNames.empty())
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
    if (!checkBuildRequirements(entity, s) || !canMorph()) {
        return false;
    }
    bool foundBp = false;
    for (auto bp : entity->getMorphedFrom()) {
        if (blueprint->getName() == bp)
            foundBp = true;
    }
    if (!foundBp)
        return false;

    s.buildActions.push_back(BuildEntityAction(entity, -1, getID(), s));
    return true;
}

EntityInst::EntityInst(const EntityBP *bp) :
    blueprint(bp),
    currentMicroEnergy(bp->getStartEnergy() * 1000000),
    morphing(false),
    id(next_id++) {
}

const EntityBP* EntityInst::getBlueprint() const { return blueprint; }
bool EntityInst::canMorph() const { return !isBusy(); }

int EntityInst::getID() const { return id; }

bool UnitInst::isBusy() const { return false; }

UnitInst::UnitInst(const UnitBP *unit) :
    EntityInst(unit) {
}

BuildingInst::BuildingInst(const BuildingBP *building) :
    EntityInst(building),
    freeBuildSlots(building->getBuildSlots()),
    chronoBoostActivated(false) {
}

bool BuildingInst::isBusy() const {
    return freeBuildSlots == 0 || EntityInst::isBusy();
}
bool BuildingInst::canMorph() const {
    return freeBuildSlots == static_cast<const BuildingBP*>(getBlueprint())->getBuildSlots() && EntityInst::canMorph();
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

    s.buildActions.push_back(BuildEntityAction(entity, -1, getID(), s));
    return true;
}

void BuildingInst::incFreeBuildSlots(){
    freeBuildSlots++;
}

ResourceInst::ResourceInst(const BuildingBP *building) :
    BuildingInst(building),
    remaining(building->startResources),
    miningRate(building->startResources.getGas() > 0 ? Resources(35, 0, 100) : Resources(0, 7, 10)),
    maxWorkerSlots(building->startResources.getGas() > 0 ? 3 : 16),
    activeWorkerSlots(0) {
}

Resources ResourceInst::mine() {
    Resources out = miningRate * activeWorkerSlots;
    if (out.getMinerals() > remaining.getMinerals())
        out.setMinerals(remaining.getMinerals());
    if (out.getGas() > remaining.getGas())
        out.setGas(remaining.getGas());
    remaining -= out;
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

bool ResourceInst::isGas() const { return (miningRate * 1000).getGas() > 0; }

bool ResourceInst::isMinerals() const { return (miningRate * 1000).getMinerals() > 0; }
void ResourceInst::copyRemaingResources(ResourceInst &other) {
    remaining = other.remaining;
    activeWorkerSlots = other.activeWorkerSlots;
}

WorkerInst::WorkerInst(const UnitBP *unit) :
    UnitInst(unit),
    workingResource(-1),
    isBuilding(false) {
}
void WorkerInst::stopMining(State &s) {
    if (workingResource != -1) {
        s.getResources().at(workingResource).removeWorker();
    }
    workingResource = -1;
}
void WorkerInst::assignToResource(ResourceInst& r, State &s){
    stopMining(s);
    workingResource = r.getID();
    r.addWorker();
}
bool WorkerInst::startBuilding(BuildingBP *bbp, State &s) {
    if (isMorphing()) {
        return false;
    }

    stopMining(s);
    if (!checkBuildRequirements(bbp, s) || !bbp->getMorphedFrom().empty()) {
        return false;
    }
    isBuilding = true;
    s.buildActions.push_back(BuildEntityAction(bbp, getID(), -1, s));
    return true;
}

void WorkerInst::stopBuilding() {
    isBuilding = false;
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
