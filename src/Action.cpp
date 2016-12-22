// vim: ts=4:sw=4 expandtab
#include "Action.h"

#include "State.h"
#include "EntityInst.h"
#include "EntityBP.h"

Action::Action(int startPoint_, int timeToFinish_) :
    startPoint(startPoint_),
    timeToFinish(timeToFinish_){
    }
void Action::tick() {
    timeToFinish -= 1;
    // TODO: Chronoboost Cuong
}
bool Action::isReady() const { return timeToFinish <= 0; }

AbilityAction::AbilityAction(const char *name_,
        const int triggeredBy_,
        const int targetBuilding_,
        int startPoint_,
        int timeToFinish_):
    Action(startPoint_, timeToFinish_),
    name(name_),
    triggeredBy(triggeredBy_),
    targetBuilding(targetBuilding_) {
}

int Action::getStartPoint() const{
    return startPoint;
}

nlohmann::json AbilityAction::printStartJSON() {
    nlohmann::json j;
    j["type"] = "special";
    j["name"] = name;
    j["triggeredBy"] = triggeredBy;
    if (targetBuilding != -1) {
        j["targetBuilding"] = targetBuilding;
    }
    return j;
}
nlohmann::json AbilityAction::printEndJSON() {
    nlohmann::json j;
    return j;
}
MuleAction::MuleAction(int startPoint_, int triggeredBy_, int worker_) :
    AbilityAction("mule", triggeredBy_, -1, startPoint_, 90),
    worker(worker_) {
}
void MuleAction::finish(State &s) {
    auto workers = s.getWorkers();
    workers.erase(workers.find(worker));
}

BuildEntityAction::BuildEntityAction(EntityBP *blueprint_ , int worker_,
        int producedBy_, State &s) :
    Action(s.timestamp,blueprint_->getBuildTime()),
    blueprint(blueprint_),
    worker(worker_),
    produced{},
    producedBy(producedBy_) {

    // change state
    s.resources -= blueprint->getCosts();

    // TODO: suppply neu berechnen

}

nlohmann::json BuildEntityAction::printStartJSON() {
    nlohmann::json j;
    j["type"] = "build-start";
    j["name"] = blueprint->getName();
    if (worker != -1) {
        j["producerID"] = worker;
    } else if (producedBy != -1){
        j["producerID"] = producedBy;
    }
    return j;
}
nlohmann::json BuildEntityAction::printEndJSON() {
    nlohmann::json j;
    j["type"] = "build-end";
    j["name"] = blueprint->getName();
    if (worker != -1) {
        j["producerID"] = worker;
    } else if (producedBy != -1){
        j["producerID"] = producedBy;
    }
    j["producedIDs"] = nlohmann::json::array();
    for (const auto ent : produced) {
        j["producedIDs"].push_back(ent);
    }
    return j;
}

void BuildEntityAction::finish(State &s) {
    bool prodUnitsInBuilding = false;
    // stop worker to build
    if(worker != -1){
        s.getWorkers().at(worker).stopBuilding();
    }
    if(producedBy != -1){
        if(s.getBuildings().find(producedBy) != s.getBuildings().end()) {
            auto building = s.getBuildings().find(producedBy);
            // increase freeBuildSlots
            building->second.incFreeBuildSlots();
            // Does the building produce units
            auto unit = dynamic_cast<const UnitBP*>(building->second.getBlueprint());
            if(unit != nullptr){
                prodUnitsInBuilding = true;
            }
            // Check if building is morphing
            if(building->second.isMorphing()){
                s.getBuildings().erase(building);
            }
        }
        // Check if worker is morphing
        else if(s.getWorkers().find(producedBy) != s.getWorkers().end()) {
            auto worker = s.getWorkers().find(producedBy);
            if(worker->second.isMorphing()){
                s.getWorkers().erase(worker);
            }
        }
        // Check if unit is morphing
        else if(s.getUnits().find(producedBy) != s.getUnits().end()) {
            auto unit = s.getUnits().find(producedBy);
            if(unit->second.isMorphing()){
                s.getUnits().erase(unit);
            }
        }
        else{
            // A resource is morphing
            auto resource = s.getResources().find(producedBy);
            if(resource == s.getResources().end()){
                std::cerr << "BuildEntityAction::finish(): Bad error. Couldn't find any instance in state to given Producer-ID" << std::endl;
            }
            auto unit = dynamic_cast<const UnitBP*>(resource->second.getBlueprint());
            if(unit != nullptr){
                prodUnitsInBuilding = true;
            }
            if(resource->second.isMorphing()){
                s.getResources().erase(resource);
            }
        }
    }
    // TODO: check if building produces two or one unit at the same time
    // include new entity in state
    int id = blueprint->newInstance(s);
    if(prodUnitsInBuilding){
        produced.push_back(id);

    }
    return;
}
