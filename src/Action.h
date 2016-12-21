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
        const int triggeredBy;
        const int targetBuilding; // optional
    protected:
        AbilityAction(const char *name_,
            const int triggeredBy_,
            const int targetBuilding_,
            int startPoint_,
            int timeToFinish_);

    public:
        nlohmann::json printStartJSON();
        nlohmann::json printEndJSON();
};
class MuleAction : public AbilityAction {
    private:
        int worker;

    public:
        MuleAction(int startPoint_, int triggeredBy_, int worker);
        void finish(State &s);
};

class BuildEntityAction : public Action {
    private:
        EntityBP* blueprint;
        int worker;
        std::vector<int> produced;
        int producedBy;

    public:
        BuildEntityAction(int startPoint_, EntityBP *blueprint_ , int worker_, int producedBy);
        nlohmann::json printStartJSON();
        nlohmann::json printEndJSON();
        void finish(State &s);
};

