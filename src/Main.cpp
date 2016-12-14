// vim: ts=4:sw=4 expandtab
#include <iostream>
#include <stdlib.h>
#include <vector>
#include <typeinfo>
#include <unordered_map>
#include <fstream>
#include <queue>

#include "EntityBP.h"
#include "EntityInst.h"
#include "State.h"

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


std::queue<EntityBP*> readBuildOrder(std::unordered_map<std::string, EntityBP> blueprints, char *fname) {
    std::queue<EntityBP*> bps;
    std::fstream input;
    input.open(fname);
    std::string line;
    while(std::getline(input, line)) {
        // TODO: push blueprints Christian
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

int main(int argc, char *argv[]) {
    std::unordered_map<std::string, EntityBP> blueprints = readConfig();
    for (size_t i = 1; i < argc; i++) {
        std::queue<EntityBP*> buildOrder = readBuildOrder(blueprints, argv[i]);
        // TODO: validateBuildOrder() Cuong

        std::vector<State> states;


        while (!buildOrder.empty()) {
            if (states.empty()) {
                states.push_back(State(buildOrder.front()->getRace(), blueprints));
            } else {
                states.push_back(states[states.size() - 1]);
            }
            State &curState = states[states.size() - 1];
            int currentTime = states.size();
            resourceUpdate(curState);

            // TODO: checkActions() Christian
            // TODO: assignUnitsToBuildings(buildOrder[0]) Cuong
            // TODO: print JSON Malte
        }
    }

	return EXIT_SUCCESS;
}
