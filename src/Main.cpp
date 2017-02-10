// vim: ts=4:sw=4 expandtab
#include <iostream>
#include <stdlib.h>
#include <vector>
#include <typeinfo>
#include <unordered_map>
#include <fstream>
#include <queue>
#include <string>
#include <random>
#include <chrono>

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
    std::fstream csv("techtree.csv", std::ios::in);
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
    if (res.empty()) {
        std::cerr << "Failed to parse CSV file." << std::endl;
        exit(EXIT_FAILURE);
    }
    return res;
}
const std::unordered_map<std::string, std::unique_ptr<EntityBP>> blueprints = readConfig();


std::deque<EntityBP*> readBuildOrder(const char *const fname) {
    std::deque<EntityBP*> bps;
    std::fstream input(fname, std::ios::in);
    if (!input.is_open()) {
        std::cerr << "failed to open input file '" << fname << "'" << std::endl;
    }
    std::string line;
    while(std::getline(input, line)) {
        auto itEntBP = blueprints.find(line);
        if(itEntBP == blueprints.end()){
            std::cerr << line << " is not a known entity." << std::endl;
            return {};
        }
        bps.push_back(itEntBP->second.get());
    }
    if (bps.empty()) {
        std::cerr << "empty input file?" << std::endl;
    }
    return bps;
}
void resourceUpdate(State &state) {
    for (auto &res : state.getResources()) {
        state.resources += res.second.mine();
    }
    state.iterEntities([&](EntityInst& ent) {
        ent.restoreEnergy();
    });
}

void larvaeUpdate(State &state) {
    static auto larvaeProducer = {
        blueprints.at("hatchery").get(),
        blueprints.at("lair").get(),
        blueprints.at("hive").get(),
    };
    for (auto res : state.getResources()) {
        auto r = std::find(larvaeProducer.begin(), larvaeProducer.end(), res.second.getBlueprint());
        if (r != larvaeProducer.end()) {
            res.second.step(state);
        }
    }
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
    actionsToJSON(curState.injectActions, events, curState.time);
    actionsToJSON(curState.chronoActions, events, curState.time);
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
    message["status"]["resources"]["supply"] = curState.computeMaxSupply() / 10;
    message["status"]["resources"]["supply-used"] = curState.computeUsedSupply() / 10;

    message["status"]["workers"]["minerals"] = mineralWorkers;
    message["status"]["workers"]["vespene"] = gasWorkers;
    message["events"] = events;
    messages.push_back(message);
}
static nlohmann::json getInitialJSON(const std::string &race,
        bool valid) {
    nlohmann::json j;
    std::string game("sc2-hots-");
    game.append(race);
    j["game"] = game;
    j["buildlistValid"] = valid ? 1 : 0;
    return j;
}
template<typename T>
static void checkActions(std::vector<T>& actions, State& s){
    for(Action& action : actions){
        action.tick(s);
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


static bool validateBuildOrder(const std::deque<EntityBP*> &initialUnits, const std::string &race) {
    State s(race, blueprints);
    std::unordered_multiset<std::string> dependencies;
    s.iterEntities([&](const EntityInst &ent) {
            dependencies.insert(ent.getBlueprint()->getName());
            });

    int vespInst = 0, bases = 1;
    int currentSupply = s.computeMaxSupply(); // 1 base = 10 supply
    int neededSupply = s.computeUsedSupply(); // six workers at begin
    for(auto bp : initialUnits) {
        if(bp->getRace() != race) {
            std::cerr << "entities to be build do not belong to one race" << std::endl;
            return false;
        }
        if(bp->getCosts().getGas() > 0 && vespInst < 1) {
            std::cerr << "Building for mining vespene missing, can not build entity which requires vespene gas: "<< bp->getName() << std::endl;
            return false;
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
        if (auto building = dynamic_cast<const BuildingBP*>(bp)) {
            // there are only two vespene geysers per base
            if(building->startResources.getGas()) {
                if(vespInst == 2*bases) {
                    std::cerr << "there are only two vespene geysers per base, cannot build : " << bp->getName() << std::endl;
                    return false;
                } else {
                    vespInst++;
                }
            } else if (building->startResources.getMinerals()) {
                bases++;
            }
        }
        auto *unit = dynamic_cast<const UnitBP*>(bp);
        if(unit != nullptr) {
            neededSupply+=unit->getSupplyCost();
        }
        currentSupply+=bp->getSupplyProvided();
        dependencies.insert(bp->getName());

    }

    if(neededSupply > currentSupply) {
        std::cerr << "not enough supplies to build new units, needed: " << neededSupply << " provided: " << currentSupply << std::endl;
        return false;
    }
    return true;
}

static bool redistributeWorkers(State &s, BuildingBP *bpToBuild) {
    bool buildingStarted = false;

    std::vector<WorkerInst *> idleWorkers;
    std::vector<WorkerInst *> mineralWorkers;
    std::vector<WorkerInst *> gasWorkers;
    for(auto& worker : s.getWorkers()) {
        if (worker.second.isMiningMinerals(s)) {
            mineralWorkers.push_back(&worker.second);
        } else if (worker.second.isMiningGas(s)) {
            gasWorkers.push_back(&worker.second);
        } else if(!worker.second.isBusy()) {
            idleWorkers.push_back(&worker.second);
        }
    }

    if (bpToBuild != nullptr) {
        // find the "best" worker to build something, and try to do so
        std::array<std::vector<WorkerInst *>*, 3> workerLists{{&idleWorkers, &mineralWorkers, &gasWorkers}};
        for (auto workers : workerLists) {
            if (!workers->empty()) {
                auto worker = workers->back();
                workers->pop_back();
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


static int simulation_all, simulation_fail;

static std::pair<std::vector<State>, nlohmann::json> simulate(std::deque<EntityBP*> &initialUnits, std::string race) {
    bool valid = !initialUnits.empty() && validateBuildOrder(initialUnits, race);
    simulation_all++;
    nlohmann::json j = getInitialJSON(race, valid);

    std::queue<EntityBP*> buildOrder(initialUnits);

    std::vector<State> states;

    if (valid) {
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
            checkActions(curState.injectActions, curState);
            checkActions(curState.chronoActions, curState);
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
            j["buildlistValid"] = 0;
            valid = false;
            /*for (EntityBP* bp : initialUnits) {
                std::cerr << bp->getName() << std::endl;
            }*/ // TODO
            simulation_fail++;
        } else {
            j["messages"] = messages;
        }
    }

    //std::cout << j.dump(4) << std::endl;
    return make_pair(states, j);
}

[[noreturn]] static void usage(char *argv[]) {
    std::cerr << "Usage: " << argv[0] << " forward  | rush UNIT TIMEOUT | push UNIT COUNT " << std::endl;
    exit(EXIT_FAILURE);
}

struct fitness {
    int targetCount;
    int timeProceeded;
};
static struct fitness get_fitness(std::vector<State> &states, UnitBP* targetBP) {
    struct fitness res = {
        0,
        0,
    };

    res.timeProceeded = states.back().time;

    for (size_t i = 0; i < states.size(); i++) {
        for (auto &unit : states[i].getUnits()) {
            if (unit.second.getBlueprint() == targetBP)
                res.targetCount++;
        }
    }

    return res;
}

struct dependency_edge {
    size_t weight;
    EntityBP *entity;
};

static std::unordered_map<EntityBP *, std::vector<dependency_edge>> generateDependencyGraph(UnitBP *targetBP, size_t count) {
    std::unordered_map<EntityBP *, std::vector<dependency_edge>> dep_graph;

    BuildingBP *gasBuilding;
    BuildingBP *mainBuilding;
    if (targetBP->getRace() == "zerg") {
        gasBuilding = static_cast<BuildingBP*>(blueprints.at("extractor").get());
        mainBuilding = static_cast<BuildingBP*>(blueprints.at("hatchery").get());
    } else if (targetBP->getRace() == "terran") {
        gasBuilding = static_cast<BuildingBP*>(blueprints.at("refinery").get());
        mainBuilding = static_cast<BuildingBP*>(blueprints.at("command_center").get());
    } else {
        gasBuilding = static_cast<BuildingBP*>(blueprints.at("assimilator").get());
        mainBuilding = static_cast<BuildingBP*>(blueprints.at("nexus").get());
    }

    std::deque<dependency_edge> worklist{{count, targetBP}};
    std::unordered_set<EntityBP*> visited;
    visited.insert(targetBP);

    auto insert_dep = [&] (EntityBP* parent, dependency_edge e, bool morph) {
        if (e.entity == mainBuilding)
            return;

        if (dep_graph.find(parent) == dep_graph.end()) {
            dep_graph.emplace(parent, std::vector<dependency_edge>{ e });
        } else {
            auto iter = std::find_if(dep_graph.at(parent).begin(), dep_graph.at(parent).end(), 
                ([&](dependency_edge &cur) {
                    return (cur.entity == e.entity);
                })
            );
            if(iter == dep_graph.at(parent).end())
                dep_graph.at(parent).push_back(e);
            else
                iter->weight += e.weight;
        }
        if (!morph && visited.find(parent) == visited.end())
            worklist.push_back({0, parent});

        visited.insert(parent);
    };

    while (!worklist.empty()) {
        dependency_edge cur = worklist.front();
        if (!cur.entity->getRequireOneOf().empty()) {
            auto front = blueprints.at(*cur.entity->getRequireOneOf().begin()).get();
            insert_dep(front, {0, cur.entity }, false);
        }
        if (!cur.entity->getProducedByOneOf().empty()) {
            auto front = blueprints.at(*cur.entity->getProducedByOneOf().begin()).get();
            insert_dep(front, { cur.weight, cur.entity }, false);
        }
        if (!cur.entity->getMorphedFrom().empty()) {
            auto front = blueprints.at(*cur.entity->getMorphedFrom().begin()).get();
            insert_dep(front, { std::max(1lu,cur.weight), cur.entity }, true);
            //if (visited.find(front) == visited.end())
            if (cur.entity != mainBuilding)
                worklist.push_back({ std::max(1lu,cur.weight), front});
        }

        if (cur.entity->getCosts().getGas() > 0) {
            insert_dep(gasBuilding, { 0, cur.entity}, false);
        }

        worklist.pop_front();
    }
    return dep_graph;
}
static void dumpDepGraph(std::unordered_map<EntityBP *, std::vector<dependency_edge>> &dep_graph) {
    std::cout << "subgraph cluster_depend{";
    for (auto &entr : dep_graph) {
        for (auto &dep : entr.second) {
            std::cout << entr.first->getName() <<
                " -> " <<
                dep.entity->getName() <<
                "[label=\"" << dep.weight << "\"]" <<
                ";";
        }
    }
    std::cout << "}";
}
static void dumpAdjGraph(std::pair<std::array<EntityBP*,1000>, std::array<bool,1000*1000>> adj_graph) {
    std::cout << "subgraph cluster_adj{";
    for(int i = 0; i < 1000; i++) {
        for(int j =0; j < 1000;j++) {
            if(adj_graph.second[i*1000+j]) {
            std::cout << adj_graph.first[i]->getName() << i <<
                " -> " <<
                adj_graph.first[j]->getName() << j <<
                ";";
            }
        }
    }
    std::cout << "}";
}

static void weightFixing(std::unordered_map<EntityBP *, std::vector<dependency_edge>> &dep_graph) {
    std::unordered_map<EntityBP *, std::pair<size_t, dependency_edge&>> incoming_edges;

    for (auto &entr : dep_graph) {
        for (dependency_edge &dep : entr.second) {
            auto p = incoming_edges.find(dep.entity);
            if( p != incoming_edges.end()){
                std::pair<size_t,dependency_edge&> value(dep.weight+p->second.first, dep);
                incoming_edges.erase(p);
                std::pair<EntityBP *, std::pair<size_t, dependency_edge &>> elem = std::make_pair(dep.entity, value);
                incoming_edges.insert(elem);
            } else {
                std::pair<size_t,dependency_edge&> value(dep.weight, dep);
                std::pair<EntityBP *, std::pair<size_t, dependency_edge &>> elem = std::make_pair(dep.entity, value);
                incoming_edges.insert(elem);
            }
        }
    }

    for(auto &p : incoming_edges){
        if(p.second.first == 0){
            p.second.second.weight = 1;
        }
    }

    return;
}

static std::pair<std::array<EntityBP*,1000>, std::array<bool,1000*1000>> graphtransformation(std::unordered_map<EntityBP *, std::vector<dependency_edge>> &dep_graph) {
    std::array<EntityBP*,1000> entities;
    entities.fill(0);
    std::array<bool,1000*1000> adjacencies;
    adjacencies.fill(0);
    std::deque<size_t> worklist;
    worklist.push_back(0);
    size_t nodeCount = 1;
    std::string race = dep_graph.begin()->second.front().entity->getRace();
    if(race == "zerg") {
        entities[0] = blueprints.at("hatchery").get();
    }
    if(race == "terran") {
        entities[0] = blueprints.at("command_center").get();
    }
    if(race == "protoss") {
        entities[0] = blueprints.at("nexus").get();
    }
    while(!worklist.empty()) {
        auto cur = worklist.front();
        worklist.pop_front();
        auto edges = dep_graph.find(entities[cur]);
        if(edges == dep_graph.end()) {
            continue;
        }
        for(auto &edge: edges->second){
            if(edge.weight > 0 && edge.weight != 0xbadf00d) {
                auto &morphFroms = edge.entity->getMorphedFrom();
                if(std::find(morphFroms.begin(), morphFroms.end(), entities[cur]->getName()) != morphFroms.end()){
                    edge.weight = edge.weight == 1 ? 0xbadf00d : edge.weight - 1;
                    worklist.push_back(nodeCount);
                    entities[nodeCount] = edge.entity;
                    adjacencies[1000*cur + nodeCount] = true;
                    nodeCount++;
                    break;
                } else {
                    for(size_t i = 0; i < edge.weight; i++) {
                        worklist.push_back(nodeCount);
                        entities[nodeCount] = edge.entity;
                        adjacencies[1000*cur + nodeCount] = true;
                        nodeCount++;
                    }
                }

            }
        }
    }

    for(size_t i = 0;i < nodeCount; i++) {
        auto edges = dep_graph.find(entities[i]);
        if(edges == dep_graph.end()) {
            continue;
        }
        for(size_t j = 0; j < nodeCount; j++) {
            for(auto & edge : edges->second) {
                if(edge.weight == 0 && edge.entity == entities[j]) {
                    adjacencies[1000*i+j] = true;
                }
            }
        }
    }
    return {entities, adjacencies};
}
// edges:  s*1000 + d
static std::vector<EntityBP*> topSort(std::mt19937 &gen, std::pair<std::array<EntityBP*,1000>, std::array<bool,1000*1000>> graph) {
    std::vector<EntityBP*> sort;
    std::vector<size_t> start_nodes{0};
    std::uniform_int_distribution<> dis_1k(0, 999);

    while (!start_nodes.empty()) {
        std::uniform_int_distribution<> dis(0, start_nodes.size() - 1);
        std::swap(start_nodes[dis(gen)], start_nodes.back());
        size_t node = start_nodes.back();
        start_nodes.pop_back();
        sort.push_back(graph.first[node]);
        size_t edge_dest = 0;
        size_t rand_offset = dis_1k(gen);
        for (size_t i = 0; i < 1000; i++) {
            size_t idx = ((i + rand_offset) % 1000);
            if (graph.second[node*1000+idx]) {
                edge_dest = idx;
                graph.second[node*1000+idx] = false;
                bool other_edge = false;
                for (size_t j = 0; edge_dest != 0 && j < 1000; j++) {
                    if (graph.second[j*1000+edge_dest]) {
                        other_edge = true;
                        break;
                    }
                }
                if (edge_dest != 0 && !other_edge) {
                    start_nodes.push_back(edge_dest);
                }
            }
        }
    }

    return sort;
}
static std::vector<EntityBP*> addUsefulStuffToBuildlist(std::mt19937 &gen, std::vector<EntityBP*> buildlist, EntityBP *targetBP, int targetCount) {
    UnitBP *worker;
    EntityBP *abilityDependency;
    EntityBP *abilityEntity;
    EntityBP *supplyEntity;
    BuildingBP *mainBuilding;
    BuildingBP *productionEntity;
    BuildingBP *gasBuilding;
    if (targetBP->getRace() == "zerg") {
        gasBuilding = static_cast<BuildingBP*>(blueprints.at("extractor").get());
        worker = static_cast<UnitBP*>(blueprints.at("drone").get());
        abilityDependency = blueprints.at("spawning_pool").get();
        abilityEntity = blueprints.at("queen").get();
        supplyEntity = blueprints.at("overlord").get();
        mainBuilding = static_cast<BuildingBP*>(blueprints.at("hatchery").get());
    } else if (targetBP->getRace() == "terran") {
        gasBuilding = static_cast<BuildingBP*>(blueprints.at("refinery").get());
        worker = static_cast<UnitBP*>(blueprints.at("scv").get());
        abilityDependency = blueprints.at("barracks").get();
        abilityEntity = blueprints.at("orbital_command").get();
        supplyEntity = blueprints.at("supply_depot").get();
        mainBuilding = static_cast<BuildingBP*>(blueprints.at("command_center").get());
    } else {
        gasBuilding = static_cast<BuildingBP*>(blueprints.at("assimilator").get());
        worker = static_cast<UnitBP*>(blueprints.at("probe").get());
        abilityDependency = nullptr;
        abilityEntity = nullptr;
        supplyEntity = blueprints.at("pylon").get();
        mainBuilding = static_cast<BuildingBP*>(blueprints.at("nexus").get());
    }

    // build additional workers
    std::uniform_int_distribution<> worker_dis(0, 21);
    std::normal_distribution<> worker_pos_dis(0, buildlist.size() / 2);
    size_t worker_count = worker_dis(gen);
    for (size_t i = 0; i < worker_count; i++) {
        size_t ins_idx = buildlist.size();
        while (ins_idx >= buildlist.size()) {
            ins_idx = std::floor(std::abs(worker_pos_dis(gen)));
        }
        buildlist.insert(buildlist.begin()+ins_idx, worker);
    }

    // when can we start building queens/orbital commands?
    size_t min_ability_idx = -1;
    size_t gas_idx = -1;
    bool want_gas = false;
    size_t first_prod_idx = -1;
    for (size_t i = 0; i < buildlist.size(); i++) {
        if (buildlist[i] == abilityDependency) {
            min_ability_idx = i + 1;
        }
        if (buildlist[i] == gasBuilding) {
            want_gas = true;
            if(gas_idx == -1)
                gas_idx = i+1;
        }
        if (!targetBP->getProducedByOneOf().empty() && buildlist[i]->getName() == targetBP->getProducedByOneOf().front()) {
            first_prod_idx = i;
        }
    }

    auto insert = [&] (size_t idx, EntityBP *what) {
        bool repeat = true;
            while (repeat) {
                repeat = false;
            if (min_ability_idx >= idx && min_ability_idx != -1)
                min_ability_idx++;
            if (gas_idx >= idx && gas_idx != -1)
                gas_idx++;
            buildlist.insert(buildlist.begin() + idx, what);
            if (!what->getMorphedFrom().empty()) {
                what = blueprints.at(what->getMorphedFrom().front()).get();
                repeat = true;
            }
        }
    };

    std::bernoulli_distribution gas_dis(want_gas ? .9 : 0);
    if(want_gas)
        insert(gas_idx, gasBuilding);

    // build additional bases + queens/orbital commands
    std::uniform_int_distribution<> main_building_dis(1, 2);
    std::uniform_int_distribution<> main_building_pos(buildlist.size()/2, buildlist.size() - 1);
    std::bernoulli_distribution ability_dis(abilityEntity == nullptr ? 0 : .9);
    size_t main_building_count = main_building_dis(gen);
    for (size_t i = 1; i < main_building_count; i++) {
        size_t idx = main_building_pos(gen);
        insert(idx, mainBuilding);
        if (ability_dis(gen) && min_ability_idx != -1) {
            insert(std::max(min_ability_idx, idx) + 1, abilityEntity);
        }
        if (gas_dis(gen))
            insert(idx + 1, gasBuilding);
        if (gas_dis(gen))
            insert(idx + 2, gasBuilding);
    }
    // first queen / orbital command
    if (ability_dis(gen) && min_ability_idx != -1) {
        insert(min_ability_idx, abilityEntity);
    }

    // additional production buildings
    std::normal_distribution<> prod_dis(targetCount / 5., 2);
    std::uniform_int_distribution<> prod_pos(first_prod_idx, buildlist.size());
    if (first_prod_idx != -1) {
        int prod_idx = 0;
        ssize_t prod_cnt = prod_dis(gen);
        for (ssize_t i = 0; i < prod_cnt; i++) {
            prod_idx = (targetBP->getProducedByOneOf().size() > 1 ? (prod_idx + 1) % 2 : 0);
            std::string producer = targetBP->getProducedByOneOf().at(prod_idx);
            insert(prod_pos(gen), blueprints.at(producer).get());
        }
    }

    // supply
    std::uniform_int_distribution<> supply_dis(1, 3);
    // Start supply depending on race
    int used_supply = 60;
    int max_supply = 100;
    for (size_t i = 0; i < buildlist.size(); i++) {

        UnitBP* unit = dynamic_cast<UnitBP*>(buildlist[i]);
        if (unit != nullptr) {
            used_supply += unit->getSupplyCost();
        }
        max_supply += buildlist[i]->getSupplyProvided();
        if(max_supply <= used_supply){
            insert(i - supply_dis(gen), supplyEntity);
            max_supply += supplyEntity->getSupplyProvided();
        }
    }

    auto larva = blueprints.at("larva").get();
    for (size_t i = 0; i < buildlist.size(); i++) {
        if (buildlist[i] == larva) {
            buildlist.erase(buildlist.begin() + i);
            i--;
        }
    }

    return buildlist;
}

static nlohmann::json optimizerLoop(std::mt19937 &gen, std::pair<std::array<EntityBP*,1000>, std::array<bool,1000*1000>> adj, UnitBP *targetBP, int targetCount, std::string mode, int timeout) {
    nlohmann::json bestList;
    int bestFitness = -1;

    auto start = std::chrono::system_clock::now();
    while(1){
        auto current = std::chrono::system_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(current - start);
        if(elapsed.count() >= timeout) {
            break;
        }
        auto newList = addUsefulStuffToBuildlist(gen, topSort(gen, adj), targetBP, targetCount);
        std::deque<EntityBP*> deqList(newList.begin(), newList.end());
        auto buildlist_info = simulate(deqList, targetBP->getRace());
        int valid = buildlist_info.second["buildlistValid"];
        if(valid){
            int curFitness;
            if(mode == "rush"){
                curFitness = get_fitness(buildlist_info.first, targetBP).targetCount;
                if(bestFitness == -1 || curFitness >= bestFitness){
                    bestList = buildlist_info.second;
                    bestFitness = curFitness;
                }
            } else {
                curFitness = get_fitness(buildlist_info.first, targetBP).timeProceeded;
                if(bestFitness == -1 || curFitness <= bestFitness){
                    bestList = buildlist_info.second;
                    bestFitness = curFitness;
                }
            }
        }
    }

    return bestList;
}

int main(int argc, char *argv[]) {
    if (argc >= 4 && std::strcmp(argv[1], "forward") == 0) {
        std::string race(argv[2]);
        for (int i = 3; i < argc; i++) {
            auto initialUnits = readBuildOrder(argv[i]);
            if(initialUnits.empty()) {
                std::cerr << "Invalid build order?" << std::endl;
            }
            simulate(initialUnits, race);
        }
    } else if (argc == 4 && std::strcmp(argv[1], "rush") == 0) {
        auto unitBP = blueprints.at(argv[2]).get();
        int timeout = std::atoi(argv[3]);
    } else if (argc == 4 && std::strcmp(argv[1], "push") == 0) {
        auto unitBP = dynamic_cast<UnitBP*>(blueprints.at(argv[2]).get());
        int count = std::atoi(argv[3]);
        auto dep_graph = generateDependencyGraph(unitBP, count);
        weightFixing(dep_graph);
        auto adj = graphtransformation(dep_graph);
        std::mt19937 gen(1337);
        auto j = optimizerLoop(gen, adj, unitBP, count, argv[1], 60);
        std::cout << j.dump(4) << std::endl;
    } else if (argc == 3 && std::strcmp(argv[1], "dump") == 0) {
        auto unitBP = dynamic_cast<UnitBP*>(blueprints.at(argv[2]).get());
        auto dep_graph = generateDependencyGraph(unitBP, 4);
        weightFixing(dep_graph);
        std::cout << "digraph {";
        dumpDepGraph(dep_graph);
        auto adj = graphtransformation(dep_graph);
        dumpAdjGraph(adj);
        std::cout << "}";

        std::mt19937 gen(1337);
        auto j = optimizerLoop(gen, adj, unitBP, 4, "push", 60);
        std::cout << j.dump(4) << std::endl;
    } else {
        usage(argv);
    }
    std::cout << simulation_fail << "/" << simulation_all << std::endl;

    return EXIT_SUCCESS;
}
