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
        const int triggeredBy_,
        const int targetBuilding_,
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
    j["triggeredBy"] = triggeredBy;
    if (targetBuilding != -1) {
        j["targetBuilding"] = targetBuilding;
    }
    return j;
}
nlohmann::json AbilityAction::printEndJSON() {
    nlohmann::json j;
    return j;
}
MuleAction::MuleAction(int startPoint_, int triggeredBy_, int worker_) :
    AbilityAction("mule", triggeredBy_, -1, startPoint_, 90),
    worker(worker_) {
}
void MuleAction::finish(State &s) {
    auto workers = s.getWorkers();
    workers.erase(workers.find(worker));
}

BuildEntityAction::BuildEntityAction(int startPoint_, EntityBP *blueprint_ , int worker_,
        int producedBy_) :
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
    if (worker != -1) {
        j["producerID"] = worker;
    } else if (producedBy != -1){
        j["producerID"] = producedBy;
    }
    return j;
}
nlohmann::json BuildEntityAction::printEndJSON() {
    nlohmann::json j;
    j["type"] = "build-end";
    j["name"] = blueprint->getName();
    if (worker != -1) {
        j["producerID"] = worker;
    } else if (producedBy != -1){
        j["producerID"] = producedBy;
    }
    j["producedIDs"] = nlohmann::json::array();
    for (const auto ent : produced) {
        j["producedIDs"].push_back(ent);
    }
    return j;
}

void BuildEntityAction::finish(State &s) {
    // stop worker to build
    if(worker != -1){
        s.getWorkers().at(worker).stopBuilding();
    }
    if(producedBy != -1){
        // increase freeBuildSlots
        auto building = s.getBuildings().find(producedBy);
        if(building != s.getBuildings().end()) {
            building->second.incFreeBuildSlots();
        // TODO: Check if entity is morphing
        }
    }
    // include new entity in state
    blueprint->newInstance(s);
    return;
}
