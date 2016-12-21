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

class AbilityAction : public Action {
    private:
        const char *const name;
        const EntityInst *const triggeredBy;
        const BuildingInst *targetBuilding; // optional
    protected:
        AbilityAction(const char *name_,
            const EntityInst *triggeredBy_,
            const BuildingInst *targetBuilding_,
            int startPoint_,
            int timeToFinish_);

    public:
        nlohmann::json printStartJSON();
        nlohmann::json printEndJSON();
};
class MuleAction : public AbilityAction {
    private:
        WorkerInst *worker;

    public:
        MuleAction(int startPoint_, EntityInst *triggeredBy_, WorkerInst *worker);
        void finish(State &s);
};

class BuildEntityAction : public Action {
    private:
        EntityBP* blueprint;
        WorkerInst* worker;
        std::vector<EntityInst *> produced;
        EntityInst* producedBy;

    public:
        BuildEntityAction(int startPoint_, EntityBP *blueprint_ , WorkerInst *worker_, EntityInst *);
        nlohmann::json printStartJSON();
        nlohmann::json printEndJSON();
        void finish(State &s);
};

