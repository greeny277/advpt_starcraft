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
    protected:
        virtual ~Action() = default;
        int timeToFinish;
    public:
        Action(int startPoint_, int timeToFinish_);
        virtual void tick(State &s);
        bool isReady() const;
        virtual nlohmann::json printStartJSON() = 0;
        virtual void printEndJSON(nlohmann::json&) = 0;
        virtual void finish(State &) = 0;
        int getStartPoint() const;
};

class AbilityAction : public Action {
    private:
        const char *name;
    protected:
        int targetBuilding; // optional
        int triggeredBy;
        AbilityAction(const char *name_,
            const int triggeredBy_,
            const int targetBuilding_,
            int startPoint_,
            int timeToFinish_);
        ~AbilityAction() = default;

    public:
        nlohmann::json printStartJSON() override;
        void printEndJSON(nlohmann::json&) override;
};
class MuleAction : public AbilityAction {
    public:
        MuleAction(int startPoint_, int triggeredBy_);
        void finish(State &s) override;
};
class ChronoAction: public AbilityAction {
    public:
        ChronoAction(int startPoint_, int triggeredBy_, int targetBuilding_);
        void finish(State &s) override;
};

class InjectAction : public AbilityAction {
    public:
        InjectAction(int startPoint_, int triggeredBy_, int targetBuilding_);
        void finish(State &s) override;
};

class BuildEntityAction : public Action {
    private:
        const EntityBP* blueprint;
        int worker;
        std::vector<int> produced;
        int producedBy;
        bool wasFinished;

    public:
        BuildEntityAction(const EntityBP *blueprint_ , int worker_, int producedBy, State &s);
        nlohmann::json printStartJSON() override;
        void printEndJSON(nlohmann::json&) override;
        void finish(State &s) override;
        inline bool hasFinished() const { return wasFinished; }
        void tick(State &s) override;
        inline const EntityBP *getBlueprint() const { return blueprint; }
};

