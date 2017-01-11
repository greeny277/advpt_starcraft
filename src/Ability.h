// vim: ts=4:sw=4 expandtab
#pragma once

#include "Action.h"
class State;

class Ability {
    public:
        const int energyCosts;
        inline explicit Ability(int energyCosts_) :
            energyCosts(energyCosts_) {
        }
        virtual bool create(State&, int triggeredBy) const = 0;
        virtual ~Ability() = default;
};

class MuleAbility : public Ability {
    public:
        MuleAbility();
        bool create(State &s, int triggeredBy) const override;
};

class InjectAbility : public Ability {
    public:
        InjectAbility();
        bool create(State &s, int triggeredBy) const override;
};
class ChronoAbility : public Ability {
    public:
        ChronoAbility();
        bool create(State &s, int triggeredBy) const override;
};
