// vim: ts=4:sw=4 expandtab
#include <iostream>
#include <stdlib.h>
#include <vector>
#include <typeinfo>
#include <unordered_map>
#include <fstream>
#include <queue>

#include "json.hpp"

#include "EntityBP.h"
#include "EntityInst.h"
#include "State.h"
#include "Action.h"
#include "Ability.h"

std::unordered_map<std::string, EntityBP*> readConfig() {
    std::unordered_map<std::string, EntityBP*> res;

    std::string line;
    std::string race;
    std::fstream csv;
    csv.open("../techtree.csv");
    while(std::getline(csv, line)) {
        if (line[0] == '#') {
            continue;
        }

        std::string cells[15];
        std::stringstream lineStream(line);
        size_t i;
        for (i = 0; std::getline(lineStream, cells[i], ',') && i < 15; i++) {
        }

        if (cells[1] == "building") {
            res.insert({cells[0], new BuildingBP(cells)});
        } else {
            res.insert({cells[0], new UnitBP(cells)});
        }


    }
    return res;
}


std::vector<EntityBP*> readBuildOrder(const std::unordered_map<std::string, EntityBP*> &blueprints, const char *const fname) {
    std::vector<EntityBP*> bps;
    std::fstream input;
    input.open(fname);
    std::string line;
    while(std::getline(input, line)) {
        auto itEntBP = blueprints.find(line);
        if(itEntBP == blueprints.end()){
            return {};
        }
        bps.push_back(itEntBP->second);
    }
    return bps;
}
void resourceUpdate(State &state) {
    state.iterEntities([&](EntityInst& ent) {
        ResourceInst* res = dynamic_cast<ResourceInst*>(&ent);
        if (res != nullptr) {
            state.resources += res->mine();
        }
        ent.restoreEnergy();
    });
}

template<typename T>
static void actionsToJSON(std::vector<T>& actions, nlohmann::json& events, int timestamp) {
    for(auto action = actions.begin(); action != actions.end(); ){
        nlohmann::json m;
        if (action->isReady()) {
            m = action->printEndJSON();
            action = actions.erase(action);
        } else if (action->getStartPoint() == timestamp) {
            m = action->printStartJSON();
            action++;
        }

        if (!m.empty()) {
            events.push_back(m);
        }
    }

}

static nlohmann::json printJSON(State &curState, int timestamp) {
    nlohmann::json message;
    message["time"] = timestamp;
    message["status"]["resources"]["minerals"] = curState.resources.getMinerals();
    message["status"]["resources"]["vespene"] = curState.resources.getGas();
    message["status"]["resources"]["supply"] = curState.currentMaxSupply;
    message["status"]["resources"]["supply-used"] = curState.currentSupply;

    int mineralWorkers = 0;
    int gasWorkers = 0;
    curState.iterEntities([&](EntityInst& entity) {
            auto res = dynamic_cast<const ResourceInst*>(&entity);
            if (res != nullptr) {
            if (res->isMinerals())
            mineralWorkers += res->getActiveWorkerCount();
            if (res->isGas())
            gasWorkers += res->getActiveWorkerCount();
            }
            });
    message["status"]["workers"]["minerals"] = mineralWorkers;
    message["status"]["workers"]["vespene"] = gasWorkers;
    auto events = nlohmann::json::array();
    actionsToJSON(curState.buildActions, events, timestamp);
    actionsToJSON(curState.buildActions, events, timestamp);
    message["events"] = events;
    return message;
}
static nlohmann::json getInitialJSON(const std::unordered_map<std::string, EntityBP*> &blueprints,
        const std::vector<EntityBP*> &initialUnits,
        const std::string &race,
        bool valid) {
    nlohmann::json j;
    std::string game("sc2-hots-");
    game.append(race);
    j["game"] = game;
    j["buildListValid"] = valid ? "1" : "0"; // WTF? why strings when JSON has booleans?

    for (const auto bp : blueprints) {
        nlohmann::json positions = nlohmann::json::array();
        for (size_t i = 0; i < initialUnits.size(); i++) {
            if (initialUnits[i] == bp.second) {
                positions.push_back(std::to_string(i)); // again, why strings???
            }
        }

        if (positions.size() > 0) {
            j["initialUnits"][bp.first] = positions;
        }
    }
    return j;
}
template<typename T>
static void checkActions(std::vector<T>& actions, State& s){
    for(Action& action : actions){
        action.tick();
        if(action.isReady()){
            action.finish(s);
        }
    }
    return;
}

static bool checkAndRunAbilities(int currentTime, State &s) {
    bool result = false;
    s.iterEntities([&](EntityInst& e) {
        for (const Ability *ab : e.getBlueprint()->getAbilities()) {
            if (e.getCurrentEnergy() >= ab->energyCosts && !result) {
                ab->create(currentTime, s, e.getID());
                e.removeEnergy(ab->energyCosts);
                result = true;
            }
        }
    });
    return result;
}

static bool buildOrderCheckOneOf(std::vector<std::string> oneOf, std::vector<std::string> dependencies) {

        if(!oneOf.empty()) {
            for(auto req: oneOf) {
                if ( std::find(dependencies.begin(), dependencies.end(), req) != dependencies.end() ) {
                    return true;
                }
            }
            return false;
        }
        return true;

}

static bool validateBuildOrder(const std::vector<EntityBP*> &initialUnits, const std::string race, const std::unordered_map<std::string, EntityBP*> &blueprints ) {
    State s(race, blueprints);
    std::vector<std::string> dependencies;
    for(auto unit: s.getUnits()) {
        dependencies.push_back(unit.second.getBlueprint()->getName());
    }
    for(auto worker: s.getWorkers()) {
        dependencies.push_back(worker.second.getBlueprint()->getName());
    }
    for(auto building: s.getBuildings()) {
        dependencies.push_back(building.second.getBlueprint()->getName());
    }
    for(auto resource: s.getResources()) {
        dependencies.push_back(resource.second.getBlueprint()->getName());
    }

    // TODO requirement vespin units can only be build of vespinInst exists
    for(auto bp : initialUnits) {
        if(bp->getRace() != race) {
            std::cerr << "entities to be build do not belong to one race" << std::endl;
            return false;
        }
        // check if the building has alle the required dependencies
        auto requireOneOf = bp->getRequireOneOf();
        bool valid = buildOrderCheckOneOf(requireOneOf, dependencies);
        if(!valid){return false;}
        
        // check if the required building for the to be produced unit exists
        auto producedByOneOf = bp->getProducedByOneOf();
        valid = buildOrderCheckOneOf(producedByOneOf, dependencies);
        if(!valid){return false;}
        
        auto morphedFrom = bp->getMorphedFrom();
        if(morphedFrom.begin()->compare("") != 0) {
            for(std::string req : morphedFrom) {
                auto position =  std::find(dependencies.begin(), dependencies.end(), req);
                if (position == dependencies.end()) {
                    std::cerr << "entity:" << bp->getName() << " cannot be upgraded, not exist yet: " << req << std::endl;
                    return false;
                } else {
                    dependencies.erase(position);
                    break;
                }
            }
        }

        dependencies.push_back(bp->getName());
    }
    return true;
}

static void redistributeWorkers(State &s, BuildingBP *bpToBuild) {
    std::vector<WorkerInst *> idleWorkers;
    std::vector<WorkerInst *> mineralWorkers;
    std::vector<WorkerInst *> gasWorkers;
    for(auto& worker : s.getWorkers()) {
        if(!worker.second.isBusy()) {
            idleWorkers.push_back(&worker.second);
        } else if (worker.second.isMiningMinerals(s)) {
            mineralWorkers.push_back(&worker.second);
        } else if (worker.second.isMiningGas(s)) {
            gasWorkers.push_back(&worker.second);
        }
    }

    if (bpToBuild != nullptr) {
        // find the "best" worker to build something, and try to do so
        std::array<std::vector<WorkerInst *>*, 3> workerLists{&idleWorkers, &mineralWorkers, &gasWorkers};
        for (auto workers : workerLists) {
            if (!workers->empty()) {
                if (workers->back()->startBuilding(bpToBuild, s)) {
                    workers->pop_back();
                }
                break;
            }
        }
    }

    std::vector<ResourceInst *> minerals;
    std::vector<ResourceInst *> gas;
    for (auto res : s.getResources()) {
        if (res.second.isMinerals()) {
            minerals.push_back(&res.second);
        } else {
            gas.push_back(&res.second);
        }
    }
    size_t workerCount = idleWorkers.size() + mineralWorkers.size() + gasWorkers.size();
    size_t gasWorkerCount = std::min(workerCount - 1, gas.size() * 3);
    size_t mineralWorkerCount = std::min(workerCount - gasWorkerCount, minerals.size() * 16);

    if (gasWorkers.size() < gasWorkerCount) {
        std::array<std::vector<WorkerInst *>*, 2> workerLists{&idleWorkers, &mineralWorkers};
        for (auto g : gas) {
            if (g->getFreeWorkerCount() == 0)
                continue;

            for (auto workers : workerLists) {
                for (auto worker : *workers) {
                    worker->assignToResource(*g);
                }
            }
        }
    }
    if (!idleWorkers.empty() && mineralWorkerCount != mineralWorkers.size()) {
        for (auto worker : idleWorkers) {
            for (auto m : minerals) {
                if (m->getFreeWorkerCount() > 0)
                    worker->assignToResource(*m);
            }
        }
    }
}

int main(int argc, char *argv[]) {
    const std::unordered_map<std::string, EntityBP*> blueprints = readConfig();
    for (size_t i = 1; i < argc; i++) {
        auto initialUnits = readBuildOrder(blueprints, argv[i]);
        if(initialUnits.empty()){
            std::cerr << "The build order list contains unknown members" << std::endl;
            return EXIT_FAILURE;
        }
        std::string race(initialUnits.front()->getRace());
        bool valid = validateBuildOrder(initialUnits, race, blueprints);
        nlohmann::json j = getInitialJSON(blueprints, initialUnits, race, valid);

        std::queue<EntityBP*, std::vector<EntityBP*>> buildOrder(initialUnits);


        if (valid) {
            std::vector<State> states;
            auto messages = nlohmann::json::array();
            j["messages"] = messages;

            while (states.size() < 1000 && !buildOrder.empty()) {
                if (states.empty()) {
                    states.push_back(State(race, blueprints));
                    j["initialUnits"] = states.back().getUnitJSON();
                } else {
                    states.push_back(states.back());
                }
                State &curState = states.back();
                // increment time attribut
                curState.time++;
                int currentTime = states.size();

                // timestep 1
                resourceUpdate(curState);

                // timestep 2
                checkActions(curState.buildActions, curState);

                // timestep 3
                checkActions(curState.muleActions, curState);
                bool canBuild = checkAndRunAbilities(currentTime, curState);

                // timestep 3.5: maybe build something
                BuildingBP *workerTask = nullptr;
                if (canBuild) {
                    auto buildNext = buildOrder.front();
                    auto unit = dynamic_cast<UnitBP*>(buildNext);
                    if (buildNext->getMorphedFrom().size()) {
                        // find a non-busy entity to upgrade/morph
                        curState.iterEntities([&](EntityInst &ent) {
                            if (!canBuild)
                                return;
                            canBuild = ent.startMorphing(buildNext, curState);
                        });
                    } else if(unit != nullptr) {
                        // find a building with where we can build the unit
                        curState.iterEntities([&](EntityInst &ent) {
                            auto building = dynamic_cast<BuildingInst*>(&ent);
                            if (!canBuild || building == nullptr)
                                return;
                            canBuild = building->produceUnit(unit, curState);
                        });
                    } else {
                        // have redistributeWorkers() build the building
                        auto building = dynamic_cast<BuildingBP*>(buildNext);
                        assert(building != nullptr);
                        workerTask = building;
                    }
                }

                // timestep 4
                redistributeWorkers(curState, workerTask);

                // timestep 5
                messages.push_back(printJSON(curState, currentTime));
            }

            if (!buildOrder.empty()) {
                valid = false;
                j.erase(j.find("messages"));
            }
        }

        std::cout << j.dump(4) << std::endl;
    }

    return EXIT_SUCCESS;
}
