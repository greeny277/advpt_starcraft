// vim: ts=4:sw=4 expandtab
#pragma once

#include "json.hpp"
#include "EntityBP.h"


class BuildingInst;
class EntityInst;
class WorkerInst;
class State;

class Action {
    private:
    int startPoint;
    int timeToFinish;
    public:
    Action(int startPoint_, int timeToFinish_);
    void tick();
    bool isReady() const;
    virtual nlohmann::json printStartJSON() = 0;
    virtual nlohmann::json printEndJSON() = 0;
    virtual void finish(State &) = 0;
    int getStartPoint() const;
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
    int startPoint_);
    // TODO Missing init of start point and timeToFinish

    public:
    inline nlohmann::json printStartJSON();
    inline nlohmann::json printEndJSON();
};
/*class MuleAction : AbilityAction {
    ConcreteWorker *mule;
};*/

class BuildingStarted : public Action {
    private:
    BuildingBP* blueprint;
    WorkerInst* worker;

    public:
    BuildingStarted(int startPoint_, BuildingBP *blueprint_ , WorkerInst *worker_);
    inline nlohmann::json printStartJSON();
    inline nlohmann::json printEndJSON();
    inline void finish(State &s);
};

class MuleAction : public Action {
    private:
        WorkerInst* mule;
        int buildingId;

    public:
        MuleAction(int startPoint_, int timeToFinish_, int buildingId_,  WorkerInst *worker);
        inline nlohmann::json printStartJSON();
        inline nlohmann::json printEndJSON();
        inline void finish(State &s);
};
