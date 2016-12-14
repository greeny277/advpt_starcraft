// vim: ts=4:sw=4 expandtab
#pragma once

class State;

class Ability {
public:
    int energyCosts;
    virtual void create(State&) = 0;
};
