// vim: ts=4:sw=4 expandtab
#pragma once

#include "json.hpp"
#include "EntityBP.h"
#include "EntityInst.h"

class State;

class Action {
    private:
    int startPoint;
    int timeToFinish;
    public:
    inline Action(int startPoint_, int timeToFinish_) :
        startPoint(startPoint_),
        timeToFinish(timeToFinish_){
    }
    inline void tick() {
        timeToFinish -= 1;
        // TODO: Chronoboost Cuong
    }
    inline bool isReady() const { return timeToFinish <= 0; }
    virtual nlohmann::json printStartJSON() const = 0;
    virtual nlohmann::json printEndJSON() const = 0;
    virtual void finish(State &) = 0;
    inline int getStartPoint() const { return startPoint; }
};

class AbilityAction : Action {
    private:
    const char *name;
    const EntityInst *triggeredBy;
    const BuildingInst *targetBuilding; // optional
    protected:
    AbilityAction(const char *name_,
    const EntityInst *triggeredBy_,
    const BuildingInst *targetBuilding_,
    int startPoint_):
    name(name_),
    triggeredBy(triggeredBy_),
    targetBuilding(targetBuilding_),
    Action(startPoint_, 0){
    }
    // TODO Missing init of start point and timeToFinish

    public:
    inline nlohmann::json printStartJSON() {
        nlohmann::json j;
        j["type"] = "special";
        j["name"] = name;
        j["triggeredBy"] = triggeredBy->getID();
        if (targetBuilding != nullptr) {
            j["targetBuilding"] = targetBuilding->getID();
        }
        return j;
    }
    inline nlohmann::json printEndJSON() {
        nlohmann::json j;
        return j;
    }
};
/*class MuleAction : AbilityAction {
    ConcreteWorker *mule;
};*/

class BuildingStarted : public Action {
    private:
    EntityBP* blueprint;
    WorkerInst* worker;

    public:
    BuildingStarted(int startPoint_, EntityBP *blueprint_ , WorkerInst *worker_) :
        blueprint(blueprint_),
        worker(worker_),
        Action(startPoint_,blueprint->getBuildTime()){
    }

    inline nlohmann::json printStartJSON() const {
        nlohmann::json j;
        j["type"] = "build-start";
        j["name"] = blueprint->getName();
        j["producerID"] = worker->getID();
        // TODO print Building ID?
        return j;
    }
    inline nlohmann::json printEndJSON() const {
        nlohmann::json j;
        j["type"] = "build-end";
        j["name"] = blueprint->getName();
        j["producerID"] = worker->getID();
        //j["producedIDs"]; // TODO malte
        // TODO print Building ID?
        return j;
    }

    inline void finish(State &s) {
        // stop worker to build
        worker->stopBuilding();

        // include new building in state
        auto building = dynamic_cast<const BuildingBP*>(blueprint);
        s.entities.push_back(new BuildingInst(building));

        return;
    }
};
