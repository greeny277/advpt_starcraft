// vim: ts=4:sw=4 expandtab
#include "Action.h"

#include "State.h"
#include "EntityInst.h"
#include "EntityBP.h"

Action::Action(int startPoint_, int timeToFinish_) :
    startPoint(startPoint_),
    timeToFinish(timeToFinish_){
    }
void Action::tick() {
    timeToFinish -= 1;
    // TODO: Chronoboost Cuong
}
bool Action::isReady() const { return timeToFinish <= 0; }

AbilityAction::AbilityAction(const char *name_,
        const EntityInst *triggeredBy_,
        const BuildingInst *targetBuilding_,
        int startPoint_,
        int timeToFinish_):
    name(name_),
    triggeredBy(triggeredBy_),
    targetBuilding(targetBuilding_),
    Action(startPoint_, timeToFinish_){
    }

int Action::getStartPoint() const{
    return startPoint;
}

nlohmann::json AbilityAction::printStartJSON() {
    nlohmann::json j;
    j["type"] = "special";
    j["name"] = name;
    j["triggeredBy"] = triggeredBy->getID();
    if (targetBuilding != nullptr) {
        j["targetBuilding"] = targetBuilding->getID();
    }
    return j;
}
nlohmann::json AbilityAction::printEndJSON() {
    nlohmann::json j;
    return j;
}
MuleAction::MuleAction(int startPoint_, EntityInst *triggeredBy_, WorkerInst *worker_) :
    AbilityAction("mule", triggeredBy_, nullptr, startPoint_, 90),
    worker(worker_) {
}
void MuleAction::finish(State &s) {
    auto it = std::find(std::begin(s.getEntities()), std::end(s.getEntities()), worker);
    assert(it != std::end(s.getEntities()));
    // move the mule to the end, then delete it
    std::swap(*it, s.getEntities().back());
    s.getEntities().pop_back();
}

BuildEntityAction::BuildEntityAction(int startPoint_, EntityBP *blueprint_ , WorkerInst *worker_,
        BuildingInst *producedBy_) :
    Action(startPoint_,blueprint_->getBuildTime()),
    blueprint(blueprint_),
    worker(worker_),
    producedBy(producedBy_),
    produced{} {
}

nlohmann::json BuildEntityAction::printStartJSON() {
    nlohmann::json j;
    j["type"] = "build-start";
    j["name"] = blueprint->getName();
    if (worker != nullptr) {
        j["producerID"] = worker->getID();
    }
    return j;
}
nlohmann::json BuildEntityAction::printEndJSON() {
    nlohmann::json j;
    j["type"] = "build-end";
    j["name"] = blueprint->getName();
    if (worker != nullptr) {
        j["producerID"] = worker->getID();
    }
    j["producedIDs"] = nlohmann::json::array();
    for (const auto ent : produced) {
        j["producedIDs"].push_back(ent->getID());
    }
    return j;
}

void BuildEntityAction::finish(State &s) {
    // stop worker to build
    if(worker != nullptr){
        worker->stopBuilding();
    }
    if(producedBy != nullptr){
        // increase freeBuildSlots
        producedBy->incFreeBuildSlots();
    }

    // include new entity in state
    s.addEntityInst(blueprint->newInstance());

    // TODO: If building was upgraded, remove former building
    // check getmorphedFrom

    return;
}
