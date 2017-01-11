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
#include "Helper.h"

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
            std::cerr << line << " is not a known entity." << std::endl;
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

void larvaeUpdate(State &state) {
    // Check current number of larvaes
    int currentLarvaes = 0;
    state.iterEntities([&](EntityInst& ent) {
        if(ent.getBlueprint()->getName() == "larva"){
            currentLarvaes++;
        }
    });
    if(currentLarvaes >= 3){
        return;
    }
    state.iterEntities([&](EntityInst& ent) {
        if(currentLarvaes == 3){
             return;
        }

        BuildingInst* build = dynamic_cast<BuildingInst*>(&ent);

        if(build != nullptr){
            auto name = build->getBlueprint()->getName();
            auto larvaeProducer = {"hatchery","lair","hive"};
            auto r = find(larvaeProducer.begin(), larvaeProducer.end(),name);
            if (r != larvaeProducer.end()) {
                if((state.time - build->getBuildTime()) % 15 == 0) {
                    // Create new larva
                    auto larva = static_cast<const UnitBP*>(state.blueprints.at("larva").get());
                    larva->newInstance(state);
                    currentLarvaes++;
                    }
                }
        }

    });
}

template<typename T>
static void actionsToJSON(std::vector<T>& actions, nlohmann::json& events, int timestamp) {
    for(auto action = actions.begin(); action != actions.end(); ){
        if (action->isReady()) {
            action->printEndJSON(events);
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

static void printJSON(State &curState, nlohmann::json &messages) {
    static int lastMineralWorkers, lastGasWorkers;
    auto events = nlohmann::json::array();
    actionsToJSON(curState.buildActions, events, curState.time);
    actionsToJSON(curState.muleActions, events, curState.time);
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
    if (events.size() == 0 && mineralWorkers == lastMineralWorkers && gasWorkers == lastGasWorkers) {
        return;
    }
    lastMineralWorkers = mineralWorkers;
    lastGasWorkers = gasWorkers;

    nlohmann::json message;
    message["time"] = curState.time;
    message["status"]["resources"]["minerals"] = curState.resources.getMinerals();
    message["status"]["resources"]["vespene"] = curState.resources.getGas();
    message["status"]["resources"]["supply"] = curState.computeMaxSupply();
    message["status"]["resources"]["supply-used"] = curState.computeUsedSupply();

    message["status"]["workers"]["minerals"] = mineralWorkers;
    message["status"]["workers"]["vespene"] = gasWorkers;
    message["events"] = events;
    messages.push_back(message);
}
static nlohmann::json getInitialJSON(const std::unordered_map<std::string, std::unique_ptr<EntityBP>> &blueprints,
        const std::deque<EntityBP*> &initialUnits,
        const std::string &race,
        bool valid) {
    nlohmann::json j;
    std::string game("sc2-hots-");
    game.append(race);
    j["game"] = game;
    j["buildlistValid"] = valid ? 1 : 0;

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


static bool validateBuildOrder(const std::deque<EntityBP*> &initialUnits, const std::string &race, const std::unordered_map<std::string, std::unique_ptr<EntityBP>> &blueprints ) {
    State s(race, blueprints);
    std::unordered_multiset<std::string> dependencies;
    s.iterEntities([&](const EntityInst &ent) {
        dependencies.insert(ent.getBlueprint()->getName());
    });

    // TODO: there are only two vespene geysers per base
    // TODO: supply
    int vespInst = 0;
    for(auto bp : initialUnits) {
        if(bp->getRace() != race) {
            std::cerr << "entities to be build do not belong to one race" << std::endl;
            return false;
        }
        if(bp->getCosts().getGas() > 0 && vespInst < 1) {
            std::cerr << "Building for mining vespene missing, can not build entity which requires vespene gas: "<< bp->getName() << std::endl;
        }
        // check if the building has all the required dependencies
        auto requireOneOf = bp->getRequireOneOf();
        bool valid = buildOrderCheckOneOf(requireOneOf, dependencies);
        if(!valid){
            std::cerr << bp->getName() << " cannot be built because of a missing requirement." << std::endl;
            return false;
        }

        // check if the required building for the to be produced unit exists
        auto producedByOneOf = bp->getProducedByOneOf();
        valid = buildOrderCheckOneOf(producedByOneOf, dependencies);
        if(!valid){
            std::cerr << bp->getName() << " cannot be built because there is no building/worker to produce it." << std::endl;
            return false;
        }

        for(const std::string &req : bp->getMorphedFrom()) {
            if( req == "larva" ) {
                /* We have an endless amount of larvaes */
                break;
            }
            auto position =  dependencies.find(req);
            if (position == dependencies.end()) {
                std::cerr << "entity:" << bp->getName() << " cannot be upgraded, " << req << " does not exist yet." << std::endl;
                return false;
            } else {
                dependencies.erase(position);
                break;
            }
        }

        dependencies.insert(bp->getName());
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
                if(race == "zerg"){
                    larvaeUpdate(curState);
                }

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
                printJSON(curState, messages);

                stillBuilding = !curState.buildActions.empty();
            }

            if (!buildOrder.empty()) {
                std::cerr << "Build order could not be finished." << std::endl;
                valid = false;
                //j["buildlistValid"] = 0;
                // TODO: activate this
                valid = false;
            } else {
                j["messages"] = messages;
            }
        }

        std::cout << j.dump(4) << std::endl;
    }

    return EXIT_SUCCESS;
}
