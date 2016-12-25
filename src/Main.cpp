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

std::unordered_map<std::string, std::unique_ptr<EntityBP>> readConfig() {
    std::unordered_map<std::string, std::unique_ptr<EntityBP>> res;

    std::string line;
    std::string race;
    std::fstream csv;
    csv.open("../techtree.csv");
    while(std::getline(csv, line)) {
        if (line[0] == '#') {
            continue;
        }

        std::string cells[16];
        std::stringstream lineStream(line);
        size_t i;
        for (i = 0; std::getline(lineStream, cells[i], ',') && i < 16; i++) {
        }

        EntityBP* ent;
        if (cells[15] == "building") {
            ent = new BuildingBP(cells);
        } else {
            ent = new UnitBP(cells);
        }
        res.emplace(cells[0], std::unique_ptr<EntityBP>(ent));

    }
    return res;
}


std::deque<EntityBP*> readBuildOrder(const std::unordered_map<std::string, std::unique_ptr<EntityBP>> &blueprints, const char *const fname) {
    std::deque<EntityBP*> bps;
    std::fstream input;
    input.open(fname);
    std::string line;
    while(std::getline(input, line)) {
        auto itEntBP = blueprints.find(line);
        if(itEntBP == blueprints.end()){
            return {};
        }
        bps.push_back(itEntBP->second.get());
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
        if (action->isReady()) {
            events.push_back(action->printEndJSON());
            std::swap(*action, actions.back());
            actions.pop_back();
        } else if (action->getStartPoint() == timestamp) {
            events.push_back(action->printStartJSON());
            action++;
        } else {
            action++;
        }
    }

}

static nlohmann::json printJSON(State &curState) {
    nlohmann::json message;
    message["time"] = curState.time;
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
    actionsToJSON(curState.buildActions, events, curState.time);
    actionsToJSON(curState.muleActions, events, curState.time);
    message["events"] = events;
    return message;
}
static nlohmann::json getInitialJSON(const std::unordered_map<std::string, std::unique_ptr<EntityBP>> &blueprints,
        const std::deque<EntityBP*> &initialUnits,
        const std::string &race,
        bool valid) {
    nlohmann::json j;
    std::string game("sc2-hots-");
    game.append(race);
    j["game"] = game;
    j["buildListValid"] = valid ? "1" : "0"; // WTF? why strings when JSON has booleans?

    for (const auto &bp : blueprints) {
        nlohmann::json positions = nlohmann::json::array();
        for (size_t i = 0; i < initialUnits.size(); i++) {
            if (initialUnits[i] == bp.second.get()) {
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

static bool checkAndRunAbilities(State &s) {
    bool result = false;
    s.iterEntities([&](EntityInst& e) {
        for (const Ability *ab : e.getBlueprint()->getAbilities()) {
            if (e.getCurrentEnergy() >= ab->energyCosts && !result) {
                if (ab->create(s, e.getID())) {
                    e.removeEnergy(ab->energyCosts);
                    result = true;
                }
            }
        }
    });
    return result;
}

static bool buildOrderCheckOneOf(std::vector<std::string> &oneOf, std::vector<std::string> &dependencies) {

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

static bool validateBuildOrder(const std::deque<EntityBP*> &initialUnits, const std::string &race, const std::unordered_map<std::string, std::unique_ptr<EntityBP>> &blueprints ) {
    State s(race, blueprints);
    std::vector<std::string> dependencies;
    s.iterEntities([&](const EntityInst &ent) {
        dependencies.push_back(ent.getBlueprint()->getName());
    });

    // TODO requirement vespin units can only be build if vespinInst exists
    // TODO: there are only two vespene geysers per base
    for(auto bp : initialUnits) {
        if(bp->getRace() != race) {
            std::cerr << "entities to be build do not belong to one race" << std::endl;
            return false;
        }
        // check if the building has all the required dependencies
        auto requireOneOf = bp->getRequireOneOf();
        bool valid = buildOrderCheckOneOf(requireOneOf, dependencies);
        if(!valid){return false;}
        
        // check if the required building for the to be produced unit exists
        auto producedByOneOf = bp->getProducedByOneOf();
        valid = buildOrderCheckOneOf(producedByOneOf, dependencies);
        if(!valid){return false;}
        
        for(const std::string &req : bp->getMorphedFrom()) {
            auto position =  std::find(dependencies.begin(), dependencies.end(), req);
            if (position == dependencies.end()) {
                std::cerr << "entity:" << bp->getName() << " cannot be upgraded, not exist yet: " << req << std::endl;
                return false;
            } else {
                dependencies.erase(position);
                break;
            }
        }

        dependencies.push_back(bp->getName());
    }
    return true;
}

static bool redistributeWorkers(State &s, BuildingBP *bpToBuild) {
    bool buildingStarted = false;

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
        std::array<std::vector<WorkerInst *>*, 3> workerLists{{&idleWorkers, &mineralWorkers, &gasWorkers}};
        for (auto workers : workerLists) {
            if (!workers->empty()) {
                auto worker = workers->back();
                workers->pop_back();
                worker->stopMining(s);
                if (worker->startBuilding(bpToBuild, s)) {
                    buildingStarted = true;
                } else {
                    idleWorkers.push_back(worker);
                }
                break;
            }
        }
    }

    std::vector<ResourceInst*> minerals;
    std::vector<ResourceInst*> gas;
    for (auto &res : s.getResources()) {
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
        std::array<std::vector<WorkerInst *>*, 2> workerLists{{&idleWorkers, &mineralWorkers}};
        for (auto g : gas) {
            for (auto workers : workerLists) {
                for (auto worker : *workers) {
                    if (g->getFreeWorkerCount() > 0) {
                        worker->assignToResource(*g, s);
                    }
                }
            }
        }
    }
    if (!idleWorkers.empty() && mineralWorkerCount != mineralWorkers.size()) {
        for (auto m : minerals) {
            for (auto worker : idleWorkers) {
                if (m->getFreeWorkerCount() > 0) {
                    worker->assignToResource(*m, s);
                }
            }
        }
    }

    return buildingStarted;
}

int main(int argc, char *argv[]) {
    const std::unordered_map<std::string, std::unique_ptr<EntityBP>> blueprints = readConfig();
    for (int i = 1; i < argc; i++) {
        auto initialUnits = readBuildOrder(blueprints, argv[i]);
        if(initialUnits.empty()){
            std::cerr << "The build order list contains unknown members" << std::endl;
            return EXIT_FAILURE;
        }
        std::string race(initialUnits.front()->getRace());
        bool valid = validateBuildOrder(initialUnits, race, blueprints);
        nlohmann::json j = getInitialJSON(blueprints, initialUnits, race, valid);

        std::queue<EntityBP*> buildOrder(initialUnits);


        if (valid) {
            std::vector<State> states;
            auto messages = nlohmann::json::array();

            bool stillBuilding = false;
            while (states.size() < 1000 && (stillBuilding || !buildOrder.empty())) {
                if (states.empty()) {
                    states.push_back(State(race, blueprints));
                    j["initialUnits"] = states.back().getUnitJSON();
                } else {
                    states.push_back(states.back());
                }
                State &curState = states.back();
                // increment time attribute
                curState.time++;

                // timestep 1
                resourceUpdate(curState);

                // timestep 2
                checkActions(curState.buildActions, curState);

                // timestep 3
                checkActions(curState.muleActions, curState);
                bool canBuild = !checkAndRunAbilities(curState);

                // timestep 3.5: maybe build something
                BuildingBP *workerTask = nullptr;
                bool buildStarted = false;
                if (canBuild && !buildOrder.empty()) {
                    auto buildNext = buildOrder.front();
                    auto unit = dynamic_cast<UnitBP*>(buildNext);
                    if (buildNext->getMorphedFrom().size()) {
                        // find a non-busy entity to upgrade/morph
                        curState.iterEntities([&](EntityInst &ent) {
                            if (buildStarted)
                                return;
                            buildStarted = ent.startMorphing(buildNext, curState);
                        });
                    } else if(unit != nullptr) {
                        // find a building where we can build the unit
                        curState.iterEntities([&](EntityInst &ent) {
                            auto building = dynamic_cast<BuildingInst*>(&ent);
                            if (buildStarted || building == nullptr)
                                return;
                            buildStarted = building->produceUnit(unit, curState);
                        });
                    } else {
                        // have redistributeWorkers() build the building
                        workerTask = dynamic_cast<BuildingBP*>(buildNext);
                        assert(workerTask != nullptr && "no idea how to build this thing");
                    }
                }

                // timestep 4
                buildStarted |= redistributeWorkers(curState, workerTask);
                if (buildStarted)
                    buildOrder.pop();

                // timestep 5
                messages.push_back(printJSON(curState));

                stillBuilding = !curState.buildActions.empty();
            }

            if (!buildOrder.empty()) {
                std::cerr << "Build order could not be finished." << std::endl;
                valid = false;
                //j["buildListValid"] = "0";
                // TODO: activate this
                j.erase(j.find("messages"));
                valid = false;
            } else {
                j["messages"] = messages;
            }
        }

        std::cout << j.dump(4) << std::endl;
    }

    return EXIT_SUCCESS;
}
