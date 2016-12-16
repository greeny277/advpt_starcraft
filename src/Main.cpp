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

std::unordered_map<std::string, EntityBP> readConfig() {
    std::unordered_map<std::string, EntityBP> res;

    std::string line;
    std::string race;
    std::fstream csv;
    csv.open("../techtree.csv");
    while(std::getline(csv, line)) {
        if (line[0] == '#') {
            continue;
        }

        //std::string name, type, minerals, vespene, buildTime, supplyCost, supplyProvided, startEnergy, maxEnergy, race, morphed_from, produced_by, dependency;
        std::string cells[12];
        std::stringstream lineStream(line);
        size_t i;
        for (i = 0; std::getline(lineStream, cells[i], ',') && i < 12; i++) {
        }

        if (cells[1] == "building") {
            res.insert({cells[0], BuildingBP(cells)});
        } else {
            res.insert({cells[0], UnitBP(cells)});
        }


    }
    return res;
}


std::vector<EntityBP*> readBuildOrder(std::unordered_map<std::string, EntityBP> blueprints, char *fname) {
    std::vector<EntityBP*> bps;
    std::fstream input;
    input.open(fname);
    std::string line;
    while(std::getline(input, line)) {
        auto itEntBP = blueprints.find(line);
        if(itEntBP == blueprints.end()){
            return {};
        }
        bps.push_back(&(itEntBP->second));
    }
    return bps;
}
void resourceUpdate(State &state) {
    for (EntityInst *ent : state.entities) {
        ResourceInst* res = dynamic_cast<ResourceInst*>(ent);
        if (res != nullptr) {
            state.resources += res->mine();
        }
    }
}

static nlohmann::json printJSON(State &curState, int timestamp) {
    nlohmann::json message;
    message["time"] = timestamp;
    message["status"]["resources"]["minerals"] = curState.resources.minerals;
    message["status"]["resources"]["vespene"] = curState.resources.gas;
    message["status"]["resources"]["supply"] = curState.currentMaxSupply;
    message["status"]["resources"]["supply-used"] = curState.currentSupply;

    int mineralWorkers = 0;
    int gasWorkers = 0;
    for (const auto & entity : curState.entities) {
        auto res = dynamic_cast<const ResourceInst*>(entity);
        if (res != nullptr) {
            if (res->isMinerals())
                mineralWorkers += res->getActiveWorkerCount();
            if (res->isGas())
                gasWorkers += res->getActiveWorkerCount();
        }
    }
    message["status"]["workers"]["minerals"] = mineralWorkers;
    message["status"]["workers"]["vespene"] = gasWorkers;

    auto action = std::begin(curState.runningActions);
    auto events = nlohmann::json::array();
    while (action != std::end(curState.runningActions)) {
        nlohmann::json m;
        if ((*action)->isReady()) {
            m = (*action)->printEndJSON();
            action = curState.runningActions.erase(action);
        } else if ((*action)->getStartPoint() == timestamp) {
            m = (*action)->printStartJSON();
            action++;
        }

        if (!m.empty()) {
            events.push_back(m);
        }
    }
    message["events"] = events;
    return message;
}
static nlohmann::json getInitialJSON(const std::unordered_map<std::string, EntityBP> &blueprints,
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
            if (initialUnits[i] == &bp.second) {
                positions.push_back(std::to_string(i)); // again, why strings???
            }
        }

        if (positions.size() > 0) {
            j["initialUnits"][bp.first] = positions;
        }
    }
    return j;
}

static void checkActions(State &s){
    for(Action *action : s.runningActions){
        action->tick();
        if(action->isReady()){
            action->finish(s);
        }
    }
    return;
}

int main(int argc, char *argv[]) {
    std::unordered_map<std::string, EntityBP> blueprints = readConfig();
    for (size_t i = 1; i < argc; i++) {
        auto initialUnits = readBuildOrder(blueprints, argv[i]);
        if(initialUnits.empty()){
            std::cerr << "The build order list contains unknown members" << std::endl;
            return EXIT_FAILURE;
        }
        std::string race(initialUnits.front()->getRace());

        // TODO: validateBuildOrder() Cuong
        bool valid = true;
        nlohmann::json j = getInitialJSON(blueprints, initialUnits, race, valid);

        std::queue<EntityBP*, std::vector<EntityBP*>> buildOrder(initialUnits);


        if (valid) {
            std::vector<State> states;

            auto messages = nlohmann::json::array();
            j["messages"] = messages;

            while (!buildOrder.empty()) {
                if (states.empty()) {
                    states.push_back(State(race, blueprints));
                } else {
                    states.push_back(states.back());
                }
                State &curState = states.back();
                int currentTime = states.size();
                resourceUpdate(curState);

                checkActions(curState); // TODO Christian
                // TODO: assignUnitsToBuildings(buildOrder[0]) Cuong

                messages.push_back(printJSON(curState, currentTime));
            }

        }

        std::cout << j.dump(4) << std::endl;
    }

	return EXIT_SUCCESS;
}
