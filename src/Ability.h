// vim: ts=4:sw=4 expandtab
#pragma once

#include "Action.h"
class State;

class Ability {
    public:
        const int energyCosts;
        inline Ability(int energyCosts_) :
            energyCosts(energyCosts_) {
        }
        virtual void create(int, State&, int triggeredBy) const = 0;
};

class MuleAbility : public Ability {
    public:
        MuleAbility();
        void create(int startPoint, State &s, int triggeredBy) const;
};
