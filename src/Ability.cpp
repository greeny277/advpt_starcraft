// vim: ts=4:sw=4 expandtab

#include "Ability.h"
#include "State.h"

MuleAbility::MuleAbility() : Ability(50) {
}

void MuleAbility::create(int startPoint, State &s, int triggeredBy) const {
    WorkerInst mule = MuleInst(static_cast<const UnitBP*>(s.blueprints.at("mule")));
    s.addWorkerInst(mule);
    s.muleActions.push_back(MuleAction(startPoint, triggeredBy, mule.getID()));
}
