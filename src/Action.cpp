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
    auto it = std::find(std::begin(s.entities), std::end(s.entities), worker);
    assert(it != std::end(s.entities));
    // move the mule to the end, then delete it
    std::swap(*it, s.entities.back());
    s.entities.pop_back();
}

BuildingStarted::BuildingStarted(int startPoint_, BuildingBP *blueprint_ , WorkerInst *worker_) :
    Action(startPoint_,blueprint_->getBuildTime()),
    blueprint(blueprint_),
    worker(worker_),
    produced{} {
}

nlohmann::json BuildingStarted::printStartJSON() {
    nlohmann::json j;
    j["type"] = "build-start";
    j["name"] = blueprint->getName();
    j["producerID"] = worker->getID();
    return j;
}
nlohmann::json BuildingStarted::printEndJSON() {
    nlohmann::json j;
    j["type"] = "build-end";
    j["name"] = blueprint->getName();
    j["producerID"] = worker->getID();
    j["producedIDs"] = nlohmann::json::array();
    for (const auto ent : produced) {
        j["producedIDs"].push_back(ent->getID());
    }
    return j;
}

void BuildingStarted::finish(State &s) {
    // stop worker to build
    worker->stopBuilding();

    // include new building in state
    // TODO: create a ResourceInst for resources
    s.entities.push_back(new BuildingInst(blueprint));

    // TODO: If building was upgraded, remove former building
    // check getmorphedFrom

    return;
}
