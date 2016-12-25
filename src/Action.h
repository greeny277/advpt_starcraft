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
    protected:
        virtual ~Action() = default;
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
        const char *name;
        int targetBuilding; // optional
    protected:
        int triggeredBy;
        AbilityAction(const char *name_,
            const int triggeredBy_,
            const int targetBuilding_,
            int startPoint_,
            int timeToFinish_);
        ~AbilityAction() = default;

    public:
        nlohmann::json printStartJSON() override;
        nlohmann::json printEndJSON() override;
};
class MuleAction : public AbilityAction {
    public:
        MuleAction(int startPoint_, int triggeredBy_);
        void finish(State &s) override;
};

class BuildEntityAction : public Action {
    private:
        EntityBP* blueprint;
        int worker;
        std::vector<int> produced;
        int producedBy;
        bool wasFinished;

    public:
        BuildEntityAction(EntityBP *blueprint_ , int worker_, int producedBy, State &s);
        nlohmann::json printStartJSON() override;
        nlohmann::json printEndJSON() override;
        void finish(State &s) override;
        inline bool hasFinished() const { return wasFinished; }
        inline EntityBP *getBlueprint() const { return blueprint; }
};

