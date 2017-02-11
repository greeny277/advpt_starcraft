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
    int targetBuilding = -1;
    for (auto &resp : s.getResources()) {
        ResourceInst& res = resp.second;
        auto name = res.getBlueprint()->getName();
        auto larvaeProducer = {"hatchery","lair","hive"};
        auto r = find(larvaeProducer.begin(), larvaeProducer.end(),name);
        if (r != larvaeProducer.end() && res.canInject()){
            targetBuilding = res.getID();
            res.startInject();
            break;
        }
    }

    if(targetBuilding != -1){
        s.injectActions.push_back(InjectAction(s.time, triggeredBy, targetBuilding));
        return true;
    } else {
        return false;
    }
}

ChronoAbility::ChronoAbility() : Ability(25) {
}

bool ChronoAbility::create(State &s, int triggeredBy) const {
    /* Find target building with producing units */
    int targetBuilding = -1;
    s.iterEntities([&] (EntityInst& ent) {
        ResourceInst *res = dynamic_cast<ResourceInst*>(&ent);
        if(res != nullptr) {
            auto name = res->getBlueprint()->getName();
            if(name == "nexus") {
                if(res->isBusy() && !res->isChronoBoosted() && targetBuilding == -1) {
                    targetBuilding = res->getID();
                    res->startChronoBoost();
                }
            }
        }
    });
    if(targetBuilding != -1) {
        s.chronoActions.push_back(ChronoAction(s.time, triggeredBy, targetBuilding));
        return true;
    } else {
        return false;
    }
}
