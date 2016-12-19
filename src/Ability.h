// vim: ts=4:sw=4 expandtab
#pragma once

#include "Action.h"
#include <unordered_map>
class State;

class Ability {
    public:
        const int energyCosts;
        inline Ability(int energyCosts_) :
            energyCosts(energyCosts_) {
        }
        virtual void create(int, State&, EntityInst *triggeredBy, const std::unordered_map<std::string, EntityBP> &blueprints) = 0;
};

class MuleAbility : public Ability {
    public:
        MuleAbility();
        void create(int startPoint, State &s, EntityInst *triggeredBy, const std::unordered_map<std::string, EntityBP> &blueprints);
};
