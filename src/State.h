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
public:
    int time;
    int currentSupply;
    Resources resources;
    int currentMaxSupply;
    std::vector<EntityInst*> entities;
    int timestamp;
    std::vector<Action*> runningActions;
    const std::unordered_map<std::string, EntityBP*> &blueprints;
    std::set<std::string> alreadyProduced; // Keeps track of entities which were produced once at least

public:
    State(const std::string &race, const std::unordered_map<std::string, EntityBP*> &blueprints);
    nlohmann::json getUnitJSON() const;
};
