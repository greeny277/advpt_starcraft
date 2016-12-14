// vim: ts=4:sw=4 expandtab
#pragma once

#include "Resources.h"

class EntityInst;
class Action;

class State {
    int currentSupply;
    Resources resources;
    int currentMaxSupply;
    std::vector<EntityInst*> entities;
    int timestamp;
    std::vector<Action*> runningActions;
};
