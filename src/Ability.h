// vim: ts=4:sw=4 expandtab
#pragma once
#include "State.h"

class Ability {
public:
    int energyCosts;
    virtual void create(State&) = 0;
};
