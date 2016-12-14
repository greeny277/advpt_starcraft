// vim: ts=4:sw=4 expandtab
#pragma once

#include <string>
#include <unordered_map>
#include "Resources.h"
#include "EntityBP.h"
#include "EntityInst.h"

class Action;

class State {
public:
    int currentSupply;
    Resources resources;
    int currentMaxSupply;
    std::vector<EntityInst*> entities;
    int timestamp;
    std::vector<Action*> runningActions;

public:
    inline State(const std::string &race, const std::unordered_map<std::string, EntityBP> &blueprints) :
            currentSupply(0),
            resources{0, 0},
            currentMaxSupply(0),
            entities{},
            timestamp(0),
            runningActions{} {

        bool mainBuilding = false;
        bool workers = false;
        for (const auto &bp : blueprints) {
            if (bp.second.getRace() == race) {
                auto building = dynamic_cast<const BuildingBP*>(&bp.second);
                if (building != nullptr && !mainBuilding) {
                    entities.push_back(new BuildingInst(building));
                    mainBuilding = true;
                }
                auto unit = dynamic_cast<const UnitBP*>(&bp.second);
                if (unit != nullptr && !workers) {
                    entities.push_back(new WorkerInst(unit));
                }
            }
        }
    }
};
