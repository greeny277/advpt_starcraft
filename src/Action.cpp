// vim: ts=4:sw=4 expandtab
#include "json.hpp"
#include "EntityBP.h"
#include "EntityInst.h"
#include "Action.h"

inline Action::Action(int startPoint_, int timeToFinish_) :
    startPoint(startPoint_),
    timeToFinish(timeToFinish_){
    }
inline void Action::tick() {
    timeToFinish -= 1;
    // TODO: Chronoboost Cuong
}
inline bool Action::isReady() const { return timeToFinish <= 0; }
virtual nlohmann::json Action::printStartJSON() const = 0;
virtual nlohmann::json Action::printEndJSON() const = 0;
virtual void Action::finish(State &) = 0;
inline int Action::getStartPoint() const { return startPoint; }

AbilityAction::AbilityAction(const char *name_,
        const EntityInst *triggeredBy_,
        const BuildingInst *targetBuilding_,
        int startPoint_):
    name(name_),
    triggeredBy(triggeredBy_),
    targetBuilding(targetBuilding_),
    Action(startPoint_, 0){
    }
// TODO Missing init of start point and timeToFinish

inline nlohmann::json AbilityAction::printStartJSON() {
    nlohmann::json j;
    j["type"] = "special";
    j["name"] = name;
    j["triggeredBy"] = triggeredBy->getID();
    if (targetBuilding != nullptr) {
        j["targetBuilding"] = targetBuilding->getID();
    }
    return j;
}
inline nlohmann::json AbilityAction::printEndJSON() {
    nlohmann::json j;
    return j;
}
/*class MuleAction : AbilityAction {
  ConcreteWorker *mule;
  };*/

BuildingStarted::BuildingStarted(int startPoint_, EntityBP *blueprint_ , WorkerInst *worker_) :
    blueprint(blueprint_),
    worker(worker_),
    Action(startPoint_,blueprint->getBuildTime()){
    }

inline nlohmann::json BuildingStarted::printStartJSON() const {
    nlohmann::json j;
    j["type"] = "build-start";
    j["name"] = blueprint->getName();
    j["producerID"] = worker->getID();
    // TODO print Building ID?
    return j;
}
inline nlohmann::json BuildingStarted::printEndJSON() const {
    nlohmann::json j;
    j["type"] = "build-end";
    j["name"] = blueprint->getName();
    j["producerID"] = worker->getID();
    //j["producedIDs"]; // TODO malte
    // TODO print Building ID?
    return j;
}

inline void BuildingStarted::finish(State &s) {
    // stop worker to build
    worker->stopBuilding();

    // include new building in state
    auto building = dynamic_cast<const BuildingBP*>(blueprint);
    s.entities.push_back(new BuildingInst(building));

    return;
}
