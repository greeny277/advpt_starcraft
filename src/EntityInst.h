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
    public:
        virtual bool isBusy() const;
        EntityInst(const EntityBP *bp);
        const EntityBP *getBlueprint() const;
        const int getID() const;
        inline int getCurrentEnergy() const { return currentMicroEnergy / 1000000; }
        inline void removeEnergy(int howMuch) { currentMicroEnergy -= howMuch * 1000000; }
        inline void restoreEnergy() { currentMicroEnergy = std::max(blueprint->getMaxEnergy() * 1000000, currentMicroEnergy + 562500); }
};
class UnitInst : public EntityInst {
    public:
         bool isBusy();
         UnitInst(const UnitBP *unit);
};

class BuildingInst : public EntityInst {
    private:
        int freeBuildSlots;
        bool chronoBoostActivated;
    public:
         BuildingInst(const BuildingBP *building);
         bool isBusy() const;
         BuildEntityAction *produceUnit(UnitBP *entity, State &s);
         void incFreeBuildSlots();
};

class ResourceInst : public BuildingInst {
    private:
        Resources remaining;
        Resources miningRate;
        int maxWorkerSlots;
        int activeWorkerSlots;
    public:
        ResourceInst(const BuildingBP *building);
        Resources mine();
        Resources getRemainingResources() const;
        /** Add/remove worker from resource and return true if
         * it was successful**/
        bool addWorker();
        bool removeWorker();

        int getActiveWorkerCount() const;
        bool isGas() const;
        bool isMinerals() const;
};
class WorkerInst : public UnitInst {
    private:
        int workingResource;
        bool isBuilding;

    public:
        WorkerInst(const UnitBP *unit);
        void assignToResource(ResourceInst& r);
        BuildEntityAction *startBuilding(BuildingBP *bbp, int curTime);
        void stopBuilding();
        bool isBusy() const;
};

class MuleInst : public WorkerInst {
    public:
        inline MuleInst(const UnitBP *unit) : WorkerInst(unit) {}
};
