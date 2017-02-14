// vim: ts=4:sw=4 expandtab
#include "Action.h"

#include "State.h"
#include "EntityInst.h"
#include "EntityBP.h"

Action::Action(int startPoint_, int timeToFinish_) :
    startPoint(startPoint_),
    timeToFinish(timeToFinish_*10){
    }
void Action::tick(State &) {
    timeToFinish -= 10;

}
bool Action::isReady() const { return timeToFinish <= 0; }

AbilityAction::AbilityAction(const char *name_,
        const int triggeredBy_,
        const int targetBuilding_,
        int startPoint_,
        int timeToFinish_):
    Action(startPoint_, timeToFinish_),
    name(name_),
    targetBuilding(targetBuilding_),
    triggeredBy(triggeredBy_) {
}

int Action::getStartPoint() const{
    return startPoint;
}

nlohmann::json AbilityAction::printStartJSON() {
    nlohmann::json j;
    j["type"] = "special";
    j["name"] = name;
    j["triggeredBy"] = std::to_string(triggeredBy);
    if (targetBuilding != -1) {
        j["targetBuilding"] = std::to_string(targetBuilding);
    }
    return j;
}
void AbilityAction::printEndJSON(nlohmann::json&) {
}
MuleAction::MuleAction(int startPoint_, int triggeredBy_) :
    AbilityAction("mule", triggeredBy_, -1, startPoint_, 90) {
}
void MuleAction::finish(State &s) {
    s.getResources().at(triggeredBy).removeMule();
}

ChronoAction::ChronoAction(int startPoint_, int triggeredBy_, int targetBuilding_):
    AbilityAction("chronoboost", triggeredBy_, targetBuilding_ , startPoint_, 20) {
}
void ChronoAction::finish(State &s) {
    ResourceInst *res = dynamic_cast<ResourceInst*>(s.getEntity(targetBuilding));
    res->stopChronoBoost();
}

InjectAction::InjectAction(int startPoint_, int triggeredBy_, int targetBuilding_) :
    AbilityAction("injectlarvae", triggeredBy_, targetBuilding_, startPoint_, 40) {
}

void InjectAction::finish(State &s){
    ResourceInst *res = dynamic_cast<ResourceInst*>(s.getEntity(targetBuilding));
    for ( int i = 0; i < 4; ++i )
    {
        res->createLarvae(s);
    }
    res->stopInject();
}


BuildEntityAction::BuildEntityAction(const EntityBP *blueprint_ , int worker_,
        int producedBy_, State &s) :
    Action(s.time, blueprint_->getBuildTime()),
    blueprint(blueprint_),
    worker(worker_),
    produced{},
    producedBy(producedBy_),
    wasFinished(false) {

    // change state
    s.resources -= blueprint->getCosts();
    if(blueprint->getRace() == ZERG && blueprint->getName() == "zergling"){
        s.adjustSupply(blueprint, true);
    }
    s.adjustSupply(blueprint, true);
}

nlohmann::json BuildEntityAction::printStartJSON() {
    nlohmann::json j;
    j["type"] = "build-start";
    j["name"] = blueprint->getName();
    if (worker != -1) {
        j["producerID"] = std::to_string(worker);
    } else if (producedBy != -1){
        j["producerID"] = std::to_string(producedBy);
    }
    return j;
}
void BuildEntityAction::printEndJSON(nlohmann::json& events) {
    nlohmann::json j;
    j["type"] = "build-end";
    j["name"] = blueprint->getName();
    if (worker != -1) {
        j["producerID"] = std::to_string(worker);
    } else if (producedBy != -1){
        j["producerID"] = std::to_string(producedBy);
    }
    j["producedIDs"] = nlohmann::json::array();
    for (const auto ent : produced) {
        j["producedIDs"].push_back(std::to_string(ent));
    }
    events.push_back(j);
}

void BuildEntityAction::tick(State &s) {
    if(producedBy != -1 && !isReady()) {
        ResourceInst *inst = dynamic_cast<ResourceInst*>(s.getEntity(producedBy));
        if(inst != nullptr) {
            if (inst->isChronoBoosted()) {
                timeToFinish-=15;
                return;
            }
        }
    }
    Action::tick(s);
}

void BuildEntityAction::finish(State &s) {
    assert(!wasFinished);
    wasFinished = true;
    if(blueprint == s.gasBuilding)
        s.gasBuildingCount++;

    if(blueprint == s.mainBuilding)
        s.mainBuildingCount++;

    EntityInst *producer = producedBy == -1 ? nullptr : s.getEntity(producedBy);
    bool morphed = producer != nullptr && producer->isMorphing();

    int id = blueprint->newInstance(s);
    int id_new = -1;
    if(blueprint->getRace() == ZERG && blueprint->getName() == "zergling"){
        id_new = blueprint->newInstance(s);
    }

    // stop worker to build
    if(worker != -1){
        if(blueprint->getRace() != PROTOSS) {
            s.getWorkers().at(worker).stopBuilding();
        }
    }
    if(producedBy != -1){
        if (producer->isMorphing()) {
            assert(!blueprint->getMorphedFrom().empty() && blueprint->getMorphedFrom().front() == producer->getBlueprint());
            if (!producer->getBlueprint()->is_unit && static_cast<const BuildingBP*>(producer->getBlueprint())->startResources.notZero()) {
                auto resource = static_cast<ResourceInst*>(producer);
                s.getResources().at(id).copyRemainingResources(*resource);
            }
            s.eraseEntity(producedBy);
        } else if (!producer->getBlueprint()->is_unit) {
            static_cast<BuildingInst*>(producer)->incFreeBuildSlots();
        }
    }

    if (morphed) {
        s.moveEntity(id, producedBy);
        id = producedBy;
    }
    produced.push_back(id); // remember ID for JSON output
    if(id_new != -1){
        produced.push_back(id_new); // remember ID for JSON output
        s.adjustSupply(blueprint, false);
    }

    s.adjustSupply(blueprint, false);

    // TODO: check if building produces two or one unit at the same time
    return;
}
