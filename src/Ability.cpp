// vim: ts=4:sw=4 expandtab

#include "Ability.h"
#include "State.h"

MuleAbility::MuleAbility() : Ability(50) {
}

void MuleAbility::create(State &s, int triggeredBy) const {
    WorkerInst mule = MuleInst(static_cast<const UnitBP*>(s.blueprints.at("mule").get()));
    s.addWorkerInst(mule);
    s.muleActions.push_back(MuleAction(s.time, triggeredBy, mule.getID()));
}
