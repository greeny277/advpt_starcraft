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

static void parseRequirements(const std::string &requirements, const std::unordered_map<std::string, std::unique_ptr<EntityBP>> &bps, std::vector<EntityBP*> &requireOneOf) {
    std::stringstream requirementStream(requirements);
    std::string req;
    while (std::getline(requirementStream, req, '/')) {
        if (!req.empty()) {
            requireOneOf.push_back(bps.at(req).get());
        }
    }
}

std::unordered_map<std::string, std::unique_ptr<EntityBP>> readConfig() {
    std::unordered_map<std::string, std::unique_ptr<EntityBP>> res;

    std::string line;
    std::string race;

    for (size_t i = 0; i < 2; i++) {
        std::fstream csv("techtree.csv", std::ios::in);
        while(std::getline(csv, line)) {
            if (line[0] == '#') {
                continue;
            }

            std::string cells[16];
            std::stringstream lineStream(line);
            for (size_t j = 0; std::getline(lineStream, cells[j], ',') && j < 16; j++) {
            }

            if (i == 0) {
                EntityBP* ent;
                if (cells[15] == "building") {
                    ent = new BuildingBP(cells);
                } else {
                    ent = new UnitBP(cells);
                }
                res.emplace(cells[0], std::unique_ptr<EntityBP>(ent));
            } else {
                std::string &name = cells[0];
                std::string requireOneOf = cells[11];
                std::string producedByOneOf = cells[10];
                std::string morphedFrom = cells[9];

                if (!morphedFrom.empty())
                    res.at(name)->getMorphedFrom().push_back(res.at(morphedFrom).get());
                parseRequirements(requireOneOf, res, res.at(name)->getRequireOneOf());
                parseRequirements(producedByOneOf, res, res.at(name)->getProducedByOneOf());
            }
        }
    }

    if (res.empty()) {
        std::cerr << "Failed to parse CSV file." << std::endl;
        exit(EXIT_FAILURE);
    }
    return res;
}
const std::unordered_map<std::string, std::unique_ptr<EntityBP>> blueprints = readConfig();


std::deque<const EntityBP*> readBuildOrder(const char *const fname) {
    std::deque<const EntityBP*> bps;
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
}

void larvaeUpdate(State &state) {
    for (auto res : state.getResources()) {
        if (res.second.isMinerals()) {
            res.second.step(state);
        }
    }
}

template<typename T>
static void actionsToJSON(std::vector<T>& actions, nlohmann::json* events, int timestamp) {
    for(auto action = actions.begin(); action != actions.end(); ){
        if (action->isReady()) {
            if (events)
                action->printEndJSON(*events);
            std::swap(*action, actions.back());
            actions.pop_back();
        } else if (action->getStartPoint() == timestamp && events) {
            events->push_back(action->printStartJSON());
            action++;
        } else {
            action++;
        }
    }

}

static void printJSON(State &curState, nlohmann::json *messages) {
    static int lastMineralWorkers, lastGasWorkers;
    auto events = nlohmann::json::array();
    actionsToJSON(curState.buildActions, messages ? &events : nullptr, curState.time);
    actionsToJSON(curState.muleActions, messages ? &events : nullptr, curState.time);
    actionsToJSON(curState.injectActions, messages ? &events : nullptr, curState.time);
    actionsToJSON(curState.chronoActions, messages ? &events : nullptr, curState.time);
    int mineralWorkers = 0;
    int gasWorkers = 0;
    for (auto &res : curState.getResources()) {
        if (res.second.isMinerals())
            mineralWorkers += res.second.getActiveWorkerCount();
        if (res.second.isGas())
            gasWorkers += res.second.getActiveWorkerCount();
    }
    if (!messages || (events.size() == 0 && mineralWorkers == lastMineralWorkers && gasWorkers == lastGasWorkers)) {
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
    messages->push_back(message);
}
static nlohmann::json getInitialJSON(const Race race, bool valid) {
    nlohmann::json j;
    std::string game("sc2-hots-");
    game.append(raceToString(race));
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
            if (e.getCurrentEnergy() >= ab->energyCosts && !result && ab->create(s, e.getID())) {
                e.removeEnergy(ab->energyCosts);
                result = true;
            }
        }
        e.restoreEnergy();
    });
    return result;
}


static bool validateBuildOrder(const std::deque<const EntityBP*> &initialUnits, const Race race, const State &s) {
    std::unordered_multiset<const EntityBP*> dependencies;
    s.iterEntities([&](const EntityInst &ent) {
        dependencies.insert(ent.getBlueprint());
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

        for(const EntityBP *req : bp->getMorphedFrom()) {
            if( req->getName() == "larva" ) {
                /* We have an endless amount of larvaes */
                break;
            }
            auto position =  dependencies.find(req);
            if (position == dependencies.end()) {
                std::cerr << "entity:" << bp->getName() << " cannot be upgraded, " << req->getName() << " does not exist yet." << std::endl;
                return false;
            } else {
                dependencies.erase(position);
                break;
            }
        }
        if (!bp->is_unit) {
            auto building = static_cast<const BuildingBP*>(bp);
            // there are only two vespene geysers per base
            if(building->startResources.getGas()) {
                if(vespInst == 2*bases) {
                    std::cerr << "there are only two vespene geysers per base, cannot build : " << bp->getName() << std::endl;
                    return false;
                } else {
                    vespInst++;
                }
            } else if (building->startResources.getMinerals() && (building->getMorphedFrom().empty() || building->getMorphedFrom().front()->is_unit)) {
                bases++;
            }
        }
        if(bp->is_unit) {
            neededSupply += static_cast<const UnitBP*>(bp)->getSupplyCost();
        }
        if (!bp->getMorphedFrom().empty()) {
            auto morph = bp->getMorphedFrom().front();
            if (morph->is_unit) {
                neededSupply -= static_cast<const UnitBP*>(morph)->getSupplyCost();
            }
        }
        currentSupply+=bp->getSupplyProvided();
        dependencies.insert(bp);

        if(neededSupply > currentSupply) {
            std::cerr << "not enough supplies to build new units, needed: " << neededSupply << " provided: " << currentSupply << std::endl;
            return false;
        }
    }
    return true;
}

static bool redistributeWorkers(State &s, const BuildingBP *bpToBuild, const std::deque<const EntityBP*> &buildQueue) {
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

    int need_gas = 0;
    for (const EntityBP* bp : buildQueue) {
        need_gas += bp->getCosts().getGas();
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

    if (!s.buildActions.empty() && s.buildActions.back().getStartPoint() == s.time) {
        need_gas -= s.buildActions.back().getBlueprint()->getCosts().getGas();
    }
    if (need_gas <= s.resources.getGas()) {
        for (auto worker : gasWorkers) {
            idleWorkers.push_back(worker);
        }
        gasWorkers.clear();
        gasWorkerCount = 0;
    }

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

static std::pair<State, bool> simulate(std::deque<const EntityBP*> &buildOrder, const Race race, int timeout, nlohmann::json *j) {
    State curState(race, blueprints);
    bool valid = !buildOrder.empty() && validateBuildOrder(buildOrder, race, curState);
    simulation_all++;
    if (j) {
        *j = getInitialJSON(race, valid);
        (*j)["initialUnits"] = curState.getUnitJSON();
    }


    if (valid) {
        nlohmann::json messages = nlohmann::json::array();

        bool stillBuilding = false;
        while (curState.time < timeout && (stillBuilding || !buildOrder.empty())) {
            // increment time attribute
            curState.time++;

            // timestep 1
            resourceUpdate(curState);
            if(race == ZERG){
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
            const BuildingBP *workerTask = nullptr;
            bool buildStarted = false;
            if (canBuild && !buildOrder.empty()) {
                auto buildNext = buildOrder.front();
                if (buildNext->getMorphedFrom().size()) {
                    // find a non-busy entity to upgrade/morph
                    curState.iterEntities([&](EntityInst &ent) {
                        if (buildStarted)
                            return;

                        buildStarted = ent.startMorphing(buildNext, curState);
                    });
                } else if(buildNext->is_unit) {
                    auto unit = static_cast<const UnitBP*>(buildNext);
                    // find a building where we can build the unit
                    curState.iterEntities([&](EntityInst &ent) {
                        if (buildStarted || ent.getBlueprint()->is_unit)
                            return;
                        auto &building = static_cast<BuildingInst&>(ent);
                        buildStarted = building.produceUnit(unit, curState);
                    });
                } else {
                    // have redistributeWorkers() build the building
                    assert(!buildNext->is_unit && "no idea how to build this thing");
                    workerTask = static_cast<const BuildingBP*>(buildNext);
                }
            }

            // timestep 4
            buildStarted |= redistributeWorkers(curState, workerTask, buildOrder);
            if (buildStarted)
                buildOrder.pop_front();

            // timestep 5
            printJSON(curState, j ? &messages : nullptr);

            stillBuilding = !curState.buildActions.empty();
        }

        if (!buildOrder.empty() || !curState.buildActions.empty()) {
            valid = false;
            if (j) {
                (*j)["buildlistValid"] = 0;
            }
            simulation_fail++;
        } else if (j) {
            (*j)["messages"] = messages;
        }
    }

    return std::make_pair(curState, valid);
}

[[noreturn]] static void usage(char *argv[]) {
    std::cerr << "Usage: " << argv[0] << " forward  | rush UNIT TIMEOUT | push UNIT COUNT " << std::endl;
    exit(EXIT_FAILURE);
}

struct fitness {
    int targetCount;
    int timeProceeded;
};
static struct fitness get_fitness(State &state, UnitBP* targetBP) {
    struct fitness res = {
        0,
        0,
    };

    res.timeProceeded = state.time;

    for (auto &unit : state.getUnits()) {
        if (unit.second.getBlueprint() == targetBP)
            res.targetCount++;
    }

    return res;
}

struct dependency_edge {
    size_t weight;
    const EntityBP *entity;
};

static std::unordered_map<const EntityBP *, std::vector<dependency_edge>> generateDependencyGraph(const UnitBP *targetBP, size_t count) {
    std::unordered_map<const EntityBP *, std::vector<dependency_edge>> dep_graph;

    const BuildingBP *gasBuilding;
    const BuildingBP *mainBuilding;
    if (targetBP->getRace() == ZERG) {
        gasBuilding = static_cast<BuildingBP*>(blueprints.at("extractor").get());
        mainBuilding = static_cast<BuildingBP*>(blueprints.at("hatchery").get());
    } else if (targetBP->getRace() == TERRAN) {
        gasBuilding = static_cast<BuildingBP*>(blueprints.at("refinery").get());
        mainBuilding = static_cast<BuildingBP*>(blueprints.at("command_center").get());
    } else {
        gasBuilding = static_cast<BuildingBP*>(blueprints.at("assimilator").get());
        mainBuilding = static_cast<BuildingBP*>(blueprints.at("nexus").get());
    }

    std::deque<dependency_edge> worklist{{count, targetBP}};
    std::unordered_set<const EntityBP*> visited{targetBP};

    auto insert_dep = [&] (const EntityBP* parent, dependency_edge e, bool morph) {
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
            auto front = cur.entity->getRequireOneOf().front();
            insert_dep(front, {0, cur.entity }, false);
        }
        if (!cur.entity->getProducedByOneOf().empty()) {
            auto front = cur.entity->getProducedByOneOf().front();
            insert_dep(front, { cur.weight, cur.entity }, false);
        }
        if (!cur.entity->getMorphedFrom().empty()) {
            auto front = cur.entity->getMorphedFrom().front();
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
/*static void dumpDepGraph(std::unordered_map<EntityBP *, std::vector<dependency_edge>> &dep_graph) {
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
}*/

static void weightFixing(std::unordered_map<const EntityBP *, std::vector<dependency_edge>> &dep_graph) {
    std::unordered_map<const EntityBP *, std::pair<size_t, dependency_edge&>> incoming_edges;

    for (auto &entr : dep_graph) {
        for (dependency_edge &dep : entr.second) {
            auto p = incoming_edges.find(dep.entity);
            if( p != incoming_edges.end()){
                std::pair<size_t,dependency_edge&> value(dep.weight+p->second.first, dep);
                incoming_edges.erase(p);
                std::pair<const EntityBP *, std::pair<size_t, dependency_edge &>> elem = std::make_pair(dep.entity, value);
                incoming_edges.insert(elem);
            } else {
                std::pair<size_t,dependency_edge&> value(dep.weight, dep);
                std::pair<const EntityBP *, std::pair<size_t, dependency_edge &>> elem = std::make_pair(dep.entity, value);
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

static std::pair<std::array<const EntityBP*,1000>, std::array<bool,1000*1000>> graphtransformation(std::unordered_map<const EntityBP *, std::vector<dependency_edge>> &dep_graph) {
    std::array<const EntityBP*,1000> entities;
    entities.fill(0);
    std::array<bool,1000*1000> adjacencies;
    adjacencies.fill(false);
    std::deque<size_t> worklist{0};
    size_t nodeCount = 1;
    assert(!dep_graph.empty());
    Race race = dep_graph.begin()->first->getRace();
    if(race == ZERG) {
        entities[0] = blueprints.at("hatchery").get();
    } else if(race == TERRAN) {
        entities[0] = blueprints.at("command_center").get();
    } else if(race == PROTOSS) {
        entities[0] = blueprints.at("nexus").get();
    }
    while(!worklist.empty()) {
        assert(nodeCount < 1000);
        auto cur = worklist.front();
        worklist.pop_front();
        auto edges = dep_graph.find(entities[cur]);
        if(edges == dep_graph.end()) {
            continue;
        }
        for(auto &edge: edges->second){
            if(edge.weight > 0 && edge.weight != 0xbadf00d) {
                auto &morphFroms = edge.entity->getMorphedFrom();
                if (!morphFroms.empty() && morphFroms[0] == entities[cur]) {
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
static void topSort(std::deque<const EntityBP*> &sort, std::mt19937 &gen, std::pair<std::array<const EntityBP*,1000>, std::array<bool,1000*1000>> graph) {
    sort.clear();
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
        for (size_t i = 0; i < 1000; i++) { // TODO: this is not particularly random
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
}
static void addUsefulStuffToBuildlist(std::mt19937 &gen, std::deque<const EntityBP*> &buildlist, EntityBP *targetBP, int targetCount) {
    const UnitBP *worker;
    const EntityBP *abilityDependency;
    const EntityBP *abilityEntity;
    const EntityBP *supplyEntity;
    const BuildingBP *mainBuilding;
    const BuildingBP *gasBuilding;
    if (targetBP->getRace() == ZERG) {
        gasBuilding = static_cast<BuildingBP*>(blueprints.at("extractor").get());
        worker = static_cast<UnitBP*>(blueprints.at("drone").get());
        abilityDependency = blueprints.at("spawning_pool").get();
        abilityEntity = blueprints.at("queen").get();
        supplyEntity = blueprints.at("overlord").get();
        mainBuilding = static_cast<BuildingBP*>(blueprints.at("hatchery").get());
    } else if (targetBP->getRace() == TERRAN) {
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

    /* Remove the main building */
    buildlist.erase(buildlist.begin());

    // when can we start building queens/orbital commands?
    ssize_t min_ability_idx = -1;
    bool want_gas = false;
    ssize_t gas_idx = -1;
    size_t first_prod_idx = ~0;
    EntityBP *first_producer = nullptr;
    if (!targetBP->getProducedByOneOf().empty())
        first_producer = targetBP->getProducedByOneOf().front();

    for (size_t i = 0; i < buildlist.size(); i++) {
        if (buildlist[i] == abilityDependency && min_ability_idx == -1) {
            min_ability_idx = i + 1;
        }
        if (buildlist[i] == gasBuilding && gas_idx == -1) {
            want_gas = true;
            gas_idx = i+1;
        }
        if (buildlist[i] == first_producer && first_prod_idx == ~ size_t { 0 }) {
            first_prod_idx = i;
        }
    }
    assert(first_prod_idx != ~ size_t { 0 } || first_producer == nullptr);

    auto insert = [&] (ssize_t idx, const EntityBP *what, bool insert_dependencies) {
        bool repeat = true;
        while (repeat) {
            repeat = false;
            if (min_ability_idx >= idx && min_ability_idx != -1)
                min_ability_idx++;
            if (gas_idx >= idx && gas_idx != -1)
                gas_idx++;
            if (first_prod_idx >= (size_t)idx && first_prod_idx != ~size_t { 0 })
                first_prod_idx++;

            buildlist.insert(buildlist.begin() + idx, what);
            if (!what->getMorphedFrom().empty()) {
                what = what->getMorphedFrom().front();
                repeat = insert_dependencies;
            }
        }
    };

    std::uniform_int_distribution<> main_building_dis(1, 3);
    std::uniform_int_distribution<> main_building_pos(buildlist.size()/2, buildlist.size() - 1);
    std::bernoulli_distribution ability_dis(abilityEntity == nullptr ? 0 : .9);
    std::bernoulli_distribution gas_dis(want_gas ? .9 : 0);
    // first queen / orbital command
    if (ability_dis(gen) && min_ability_idx != -1) {
        insert(min_ability_idx, abilityEntity, true);
        if(abilityEntity->getCosts().getGas() > 0){
            want_gas = true;
            gas_idx = min_ability_idx-1;
        }
    }
    if(gas_dis(gen))
        insert(gas_idx, gasBuilding, true);

    size_t extrac_cnt = 0;
    for (size_t i = 0; i < buildlist.size(); i++) {
        if (buildlist[i] == gasBuilding)
            extrac_cnt++;
    }
    assert(extrac_cnt < 3);

    // build additional bases + queens/orbital commands
    size_t main_building_count = main_building_dis(gen);
    for (size_t i = 1; i < main_building_count; i++) {
        ssize_t idx = main_building_pos(gen);
        if (ability_dis(gen) && min_ability_idx != -1) {
            insert(std::max(min_ability_idx, idx), abilityEntity, true);
        }
        if (gas_dis(gen))
            insert(idx, gasBuilding, true);
        if (gas_dis(gen))
            insert(idx, gasBuilding, true);
        insert(idx, mainBuilding, true);
    }

    extrac_cnt = 0;
    for (size_t i = 0; i < buildlist.size(); i++) {
        if (buildlist[i] == gasBuilding)
            extrac_cnt++;
    }
    assert(extrac_cnt / 2 <= main_building_count);

    // additional production buildings
    std::normal_distribution<double> prod_dis(0, std::max(targetCount / 3., 2.));
    if (first_prod_idx != ~size_t { 0 }) {
        std::uniform_int_distribution<> prod_pos(first_prod_idx, buildlist.size() - 1);
        double prod_cnt = std::round(prod_dis(gen));
        std::multiset<size_t> prod_indices;
        for (size_t i = 0; i < prod_cnt; i++) {
            prod_indices.insert(prod_pos(gen));
        }
        size_t i = 1;
        for (size_t pos : prod_indices) {
            size_t prod_idx = targetBP->getProducedByOneOf().size() > 1 ? (i % 2) : 0;
            EntityBP *producer = targetBP->getProducedByOneOf().at(prod_idx);
            insert(pos + i, producer, false);
            i++;
        }
    }

    // build additional workers
    std::gamma_distribution<> worker_dis(7, 1.0);
    std::normal_distribution<> worker_pos_dis(0, buildlist.size() / 2);
    size_t worker_count = std::min(size_t { 21 }, (size_t)std::floor(worker_dis(gen)));
    for (size_t i = 0; i < worker_count; i++) {
        size_t ins_idx = buildlist.size();
        while (ins_idx >= buildlist.size()) {
            ins_idx = std::floor(std::abs(worker_pos_dis(gen)));
        }
        insert(ins_idx, worker, false);
    }

    // supply
    std::uniform_int_distribution<> supply_dis(1, 3);
    int used_supply = 60;
    int max_supply = 100;
    for (ssize_t i = 0; i < (ssize_t)buildlist.size(); i++) {
        if(!want_gas && buildlist[i]->getCosts().getGas() > 0){
            insert(std::max(ssize_t { 0l }, i - supply_dis(gen)), gasBuilding, true);
            want_gas = true;
        }

        if (buildlist[i]->is_unit) {
            used_supply += static_cast<const UnitBP*>(buildlist[i])->getSupplyCost();
            if (buildlist[i]->getName() == "zergling") {
                used_supply += static_cast<const UnitBP*>(buildlist[i])->getSupplyCost();
            }
        }
        if (!buildlist[i]->getMorphedFrom().empty()) {
            EntityBP *morph = buildlist[i]->getMorphedFrom().front();
            if (morph->is_unit) {
                used_supply -= static_cast<UnitBP*>(morph)->getSupplyCost();
            }
            max_supply -= morph->getSupplyProvided();
        }
        max_supply += buildlist[i]->getSupplyProvided();
        if(max_supply < used_supply) {
            insert(std::max(ssize_t { 0 }, i - supply_dis(gen)), supplyEntity, true);
            max_supply += supplyEntity->getSupplyProvided();
        }
    }
    if (used_supply > 2000)
        buildlist.clear();

    auto larva = blueprints.at("larva").get();
    int gas_counter = 0;
    int main_building_counter = 1;
    for (size_t i = 0; i < buildlist.size(); i++) {
        if(buildlist[i] == mainBuilding){
            main_building_counter++;
            continue;
        }

        if(buildlist[i] == gasBuilding){
            gas_counter++;
            if(gas_counter > main_building_counter*2){
                buildlist.erase(buildlist.begin() + i);
                gas_counter--;
                i--;
            }
            continue;
        }
        if (buildlist[i] == larva) {
            buildlist.erase(buildlist.begin() + i);
            i--;
            continue;
        }
    }
}

static nlohmann::json optimizerLoop(std::mt19937 &gen, UnitBP *targetBP, int targetCount, std::string mode, int timeout, int timebarrier) {
    std::deque<const EntityBP*> bestBuildList, curList;

    int bestFitness = -1;
    int curCount = targetCount;
    auto dep_graph = generateDependencyGraph(targetBP, curCount);
    weightFixing(dep_graph);
    auto adj = graphtransformation(dep_graph);
    bool rush_mode = mode == "rush";

    auto start = std::chrono::system_clock::now();
    while(1){
        auto current = std::chrono::system_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(current - start);
        if(elapsed.count() >= timeout) {
            break;
        }
        std::mt19937 gen_copy(gen);
        topSort(curList, gen, adj);
        addUsefulStuffToBuildlist(gen, curList, targetBP, targetCount);
        if (curList.empty())
            continue;

        auto buildlist_info = simulate(curList, targetBP->getRace(), timebarrier, nullptr);
        if(buildlist_info.second){
            int curFitness = get_fitness(buildlist_info.first, targetBP).timeProceeded;
            bool best = false;
            if(rush_mode && curFitness <= timebarrier){
                best = true;
            } else if(!rush_mode && (bestFitness == -1 || curFitness < bestFitness)) {
                best = true;
                bestFitness = curFitness;
            }

            if (best) {
                topSort(bestBuildList, gen_copy, adj);
                addUsefulStuffToBuildlist(gen_copy, bestBuildList, targetBP, targetCount);

                if (rush_mode) {
                    // try generating even more units
                    curCount++;
                    dep_graph = generateDependencyGraph(targetBP, curCount);
                    weightFixing(dep_graph);
                    adj = graphtransformation(dep_graph);
                }

            }
        }
    }
    std::cerr << "Builded units: " << curCount-1 << std::endl;
    for (const EntityBP* bp : bestBuildList)
      std::cerr << bp->getName() << std::endl;

    nlohmann::json bestList;
    auto r = simulate(bestBuildList, targetBP->getRace(), timebarrier, &bestList);
    assert(r.second);

    return bestList;
}

#define TIMEOUT 179
#define SEED 1337

int main(int argc, char *argv[]) {
    if (argc >= 4 && std::strcmp(argv[1], "forward") == 0) {
        Race race = raceFromString(std::string{argv[2]});
        for (int i = 3; i < argc; i++) {
            auto initialUnits = readBuildOrder(argv[i]);
            if(initialUnits.empty()) {
                std::cerr << "Invalid build order?" << std::endl;
            }
            nlohmann::json j;
            simulate(initialUnits, race, 1000, &j);
            std::cout << j.dump(4) << std::endl;
        }
    } else if (argc == 4 && std::strcmp(argv[1], "rush") == 0) {
        auto unitBP = dynamic_cast<UnitBP*>(blueprints.at(argv[2]).get());
        int timebarrier = std::atoi(argv[3]);
        std::mt19937 gen(SEED);
        auto j = optimizerLoop(gen, unitBP, 1, argv[1], TIMEOUT, timebarrier);
        std::cout << j.dump(4) << std::endl;
    } else if (argc == 4 && std::strcmp(argv[1], "push") == 0) {
        auto unitBP = dynamic_cast<UnitBP*>(blueprints.at(argv[2]).get());
        int count = std::atoi(argv[3]);
        std::mt19937 gen(SEED);
        auto j = optimizerLoop(gen, unitBP, count, argv[1], TIMEOUT, 1000);
        std::cout << j.dump(4) << std::endl;
    } else {
        usage(argv);
    }
    //std::cout << simulation_fail << "/" << simulation_all << std::endl;

    return EXIT_SUCCESS;
}
