// vim: ts=4:sw=4 expandtab
#pragma once

#include <string>
#include <unordered_map>
#include <set>
#include "Resources.h"
#include "EntityBP.h"
#include "EntityInst.h"
#include "Action.h"
#include "json.hpp"


class State {
private:
    std::vector<EntityInst*> entities;
public:
    int time;
    int currentSupply;
    Resources resources;
    int currentMaxSupply;
    int timestamp;
    std::vector<Action*> runningActions;
    const std::unordered_map<std::string, EntityBP*> &blueprints;
    std::set<std::string> alreadyProduced; // Keeps track of entities which were produced once at least

public:
    State(const std::string &race, const std::unordered_map<std::string, EntityBP*> &blueprints);
    nlohmann::json getUnitJSON() const;
    // TODO The returning vector should be const later on
    std::vector<EntityInst*>& getEntities();
    void addEntityInst(EntityInst *);
};
