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
            const BuildingInst *targetBuilding_):
            name(name_),
            triggeredBy(triggeredBy_),
            targetBuilding(targetBuilding_) {
    }

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
    inline virtual nlohmann::json printStartJSON() {
        nlohmann::json j;
        j["type"] = "build-start";
        j["name"] = blueprint->getName();
        j["producerID"] = worker->getID();
        return j;
    }
    inline virtual nlohmann::json printEndJSON() {
        nlohmann::json j;
        j["type"] = "build-start";
        j["name"] = blueprint->getName();
        j["producerID"] = worker->getID();
        //j["producedIDs"]; // TODO malte
        return j;
    }
    inline virtual void finish(State &) {
        // TODO Christian
    }
};
