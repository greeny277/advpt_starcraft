// vim: ts=4:sw=4 expandtab
#pragma once

#include "EntityBP.h"
#include "Resources.h"
#include "Action.h"

class BuildingStarted;
class State;

class EntityInst {
    private:
        const EntityBP *blueprint;
        int currentMicroEnergy;
        bool morphing; // when the unit is currently upgrading, the entity cannot do anything else
        const int id;
        static int next_id;
    protected:
        bool checkBuildRequirements(EntityBP*, State&);
    public:
        virtual bool isBusy() const;
        bool isMorphing() const;
        void setMorphing(bool b);
        bool startMorphing(EntityBP *, State &s);
        explicit EntityInst(const EntityBP *bp);
        const EntityBP *getBlueprint() const;
        int getID() const;
        inline int getCurrentEnergy() const { return currentMicroEnergy / 1000000; }
        inline void removeEnergy(int howMuch) { currentMicroEnergy -= howMuch * 1000000; }
        inline void restoreEnergy() { currentMicroEnergy = std::max(blueprint->getMaxEnergy() * 1000000, currentMicroEnergy + 562500); }
        virtual bool canMorph() const;
};
class UnitInst : public EntityInst {
    public:
         bool isBusy() const override;
         explicit UnitInst(const UnitBP *unit);
};

class BuildingInst : public EntityInst {
    private:
        int freeBuildSlots;
        bool chronoBoostActivated;
    public:
         explicit BuildingInst(const BuildingBP *building);
         bool isBusy() const override;
         bool produceUnit(UnitBP *entity, State &s);
         void incFreeBuildSlots();
         bool canMorph() const override;
};

class ResourceInst : public BuildingInst {
    private:
        Resources remaining;
        const Resources miningRate;
        const int maxWorkerSlots;
        int activeWorkerSlots;
    public:
        explicit ResourceInst(const BuildingBP *building);
        Resources mine();
        Resources getRemainingResources() const;
        /** Add/remove worker from resource and return true if
         * it was successful**/
        bool addWorker();
        bool removeWorker();

        int getActiveWorkerCount() const;
        int getFreeWorkerCount() const;
        bool isGas() const;
        bool isMinerals() const;
        void copyRemaingResources(ResourceInst &other);
};
class WorkerInst : public UnitInst {
    private:
        int workingResource;
        bool isBuilding;

    public:
        explicit WorkerInst(const UnitBP *unit);
        void assignToResource(ResourceInst& r, State&);
        void stopMining(State &s);
        bool startBuilding(BuildingBP *bbp, State&);
        void stopBuilding();
        bool isBusy() const override;
        bool isMiningMinerals(State&) const;
        bool isMiningGas(State&) const;
};

class MuleInst : public WorkerInst {
    public:
        explicit inline MuleInst(const UnitBP *unit) : WorkerInst(unit) {}
};
