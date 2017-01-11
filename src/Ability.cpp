// vim: ts=4:sw=4 expandtab

#include "Ability.h"
#include "State.h"

MuleAbility::MuleAbility() : Ability(50) {
}

bool MuleAbility::create(State &s, int triggeredBy) const {
    bool ok = s.getResources().at(triggeredBy).addMule();
    if (ok) {
        s.muleActions.push_back(MuleAction(s.time, triggeredBy));
    }
    return ok;
}

InjectAbility::InjectAbility() : Ability(25) {
}

bool InjectAbility::create(State &s, int triggeredBy) const {
    /* Find a fitting hatchery with the biggest amount of larvae slots */
    return false;
}

ChronoAbility::ChronoAbility() : Ability(25) {
}
bool ChronoAbility::create(State &s, int triggeredBy) const {
    /* Find a fitting building to activate the chrono boost */
    return false;
}
