// vim: ts=4:sw=4 expandtab

#include "Ability.h"
#include "State.h"

MuleAbility::MuleAbility() : Ability(50) {
}

void MuleAbility::create(int startPoint, State &s, EntityInst *triggeredBy, const std::unordered_map<std::string, EntityBP> &blueprints) {
    WorkerInst *mule = new MuleInst(static_cast<const UnitBP*>(&blueprints.at("mule")));
    s.entities.push_back(mule);
    s.runningActions.push_back(new MuleAction(startPoint, triggeredBy, mule));
}
