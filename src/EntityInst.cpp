// vim: ts=4:sw=4 expandtab

#include "EntityInst.h"

#include "Action.h"
#include "State.h"
#include "Helper.h"


int EntityInst::next_id = 0;

bool EntityInst::isBusy() const {
    return morphing;
}

bool EntityInst::checkBuildRequirements(EntityBP *entity, State &s, const UnitBP *morphedFrom) {
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
    const auto &buildingNames = entity->getProducedByOneOf();
    if (!buildingNames.empty() && std::find(buildingNames.begin(), buildingNames.end(), getBlueprint()->getName()) == buildingNames.end()) {
        // this building can not produce the unit
        return false;
    }

    // check requirements
    if (!buildOrderCheckOneOf(entity->getRequireOneOf(), s.alreadyProduced)) {
        return false;
    }

    // check supply
    if (auto unit = dynamic_cast<UnitBP *>(entity)) {
        int supplyUsed = s.computeUsedSupply();
        supplyUsed += unit->getSupplyCost();
        if (morphedFrom != nullptr) {
            supplyUsed -= morphedFrom->getSupplyCost();
        }
        if (supplyUsed > s.computeMaxSupply())
            return false;
    }

    return true;
}
bool EntityInst::isMorphing() const {
    return morphing;
}

bool EntityInst::startMorphing(EntityBP *entity, State &s) {
    if (!checkBuildRequirements(entity, s, dynamic_cast<const UnitBP*>(getBlueprint())) || !canMorph())
    {
        return false;
    }
    bool foundBp = false;
    for (auto bp : entity->getMorphedFrom()) {
        if (blueprint->getName() == bp)
            foundBp = true;
    }
    if (!foundBp){
        return false;
    }
    if (auto worker = dynamic_cast<WorkerInst *>(this)) {
        worker->stopMining(s);
    }

    s.buildActions.push_back(BuildEntityAction(entity, -1, getID(), s));
    morphing = true;

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
void EntityInst::setID(int id_) { id = id_; }

UnitInst::UnitInst(const UnitBP *unit) :
    EntityInst(unit) {
}

BuildingInst::BuildingInst(const BuildingBP *building) :
    EntityInst(building),
    freeBuildSlots(building->getBuildSlots()){
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
    if (!checkBuildRequirements(entity, s, nullptr) || !entity->getMorphedFrom().empty()) {
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
    timerActive(false),
    larvaeTimer(0),
    inject(false),
    larvaeIds({}),
    remaining(building->startResources),
    miningRate(building->startResources.getGas() > 0 ? Resources(35, 0, 100) : Resources(0, 7, 10)),
    maxWorkerSlots(building->startResources.getGas() > 0 ? 3 : 16),
    activeWorkerSlots(0),
    activeMuleSlots(0),
    chronoBoostActivated(false) {
}

Resources ResourceInst::mine() {
    Resources out = miningRate * (activeWorkerSlots + 4 * activeMuleSlots);
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

bool ResourceInst::addWorker() {
    if(activeWorkerSlots == maxWorkerSlots){
        return false;
    }
    activeWorkerSlots++;
    return true;
}

void ResourceInst::removeWorker(){
    assert(activeWorkerSlots > 0);
    activeWorkerSlots--;
}
bool ResourceInst::addMule() {
    assert(isMinerals());
    if (activeMuleSlots + 1 >= maxWorkerSlots / 2)
        return false;
    activeMuleSlots++;
    return true;
}
void ResourceInst::removeMule() {
    assert(isMinerals());
    assert(activeMuleSlots > 0);
    activeMuleSlots--;
}

void ResourceInst::step(State &s){
    removeMorphingLarvae(s);
    if(!getFreeLarvaeCount()){
        stopTimer();
        return;
    } else {
        larvaeTimer = (larvaeTimer+1) % 15;
        if (larvaeTimer == 0) {
            createLarvae(s);
            if(!getFreeLarvaeCount()){
                stopTimer();
            }
        }
    }
}
bool ResourceInst::getFreeLarvaeCount() const{
    return larvaeIds.size() < 3;
}

void ResourceInst::createLarvae(State &s){
    auto larva = static_cast<const UnitBP*>(s.blueprints.at("larva").get());
    larvaeIds.push_back(larva->newInstance(s));
}

void ResourceInst::removeMorphingLarvae(State &s){
    for (auto it = larvaeIds.begin() ; it != larvaeIds.end(); ) {
        if(s.getEntity(*it)->isMorphing()){
            it = larvaeIds.erase(it);
        } else {
            ++it;
        }
    }
    if (!timerActive){
        startTimer();
    }
}

void ResourceInst::startTimer(){
    timerActive = true;
    larvaeTimer = 0;
}

void ResourceInst::stopTimer(){
    timerActive = false;
    larvaeTimer = 0;
}

void ResourceInst::startChronoBoost() {
    chronoBoostActivated = true;
}
void ResourceInst::stopChronoBoost() {
    chronoBoostActivated = false;
}

bool ResourceInst::isChronoBoosted() {
    return chronoBoostActivated;
}

bool ResourceInst::canInject(){
    return !inject;
}

void ResourceInst::startInject(){
    inject = true;
}

void ResourceInst::stopInject(){
    inject = false;
}

int ResourceInst::getActiveWorkerCount() const { return activeWorkerSlots; }
int ResourceInst::getFreeWorkerCount() const { return maxWorkerSlots - activeWorkerSlots; }

bool ResourceInst::isGas() const { return (miningRate * 1000).getGas() > 0; }

bool ResourceInst::isMinerals() const { return (miningRate * 1000).getMinerals() > 0; }
void ResourceInst::copyRemainingResources(ResourceInst &other) {
    remaining = other.remaining;
    activeMuleSlots = other.activeMuleSlots;
    int workers = other.activeWorkerSlots;
    activeWorkerSlots = workers;
    other.activeWorkerSlots = 0;
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
    if(getBlueprint()->getName() != "probe") {
        stopMining(s);
    }
    if (!checkBuildRequirements(bbp, s, nullptr) || !bbp->getMorphedFrom().empty()) {
        return false;
    }
    if(getBlueprint()->getName() != "probe") {
        isBuilding = true;
    }
    s.buildActions.push_back(BuildEntityAction(bbp, getID(), -1, s));
    return true;
}

void WorkerInst::stopBuilding() {
    isBuilding = false;
}
bool WorkerInst::isBusy() const { return isBuilding || isMorphing(); }
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

bool WorkerInst::isAssignedTo(int id_) const {
    return id_ == workingResource;
}
