// vim: ts=4:sw=4 expandtab
#pragma once

#include "Action.h"
class State;

class Ability {
public:
    int duration;
    int energyCosts;
    EntityInst *triggeredBy;
    virtual void create(int, State&) = 0;
};

class Mule : public Ability {
    public:
    Ability(int energyCosts);
    MuleAction create(int startPoint, State &s);
};
