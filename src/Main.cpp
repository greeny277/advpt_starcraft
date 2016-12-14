// vim: ts=4:sw=4 expandtab
#include <iostream>
#include <stdlib.h>
#include <vector>
#include <typeinfo>
#include <unordered_map>
#include <fstream>
#include "EntityBP.h"

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
    std::fstream input;
    input.open(fname);
    std::string line;
    while(std::getline(input, line)) {
        // TODO: push blueprints Christian
    }
}

int main(int argc, char *argv[]) {
    std::unordered_map<std::string, EntityBP> blueprints = readConfig();
    for (size_t i = 0; i < argc; i++) {
        std::queue<EntityBP*> buildOrder = readBuildOrder(blueprints, argv[1]);
        // TODO: validateBuildOrder() Cuong

        std::vector<State> states;


        while (!buildOrder.empty()) {
            // TODO: clone last state Malte
            // TODO: increase time stamp Malte
            // TODO: resourceUpdate() Malte


            // TODO: checkActions() Christian
            // TODO: assignUnitsToBuildings(buildOrder[0]) Cuong
            // TODO: print JSON Malte
        }
    }

	return EXIT_SUCCESS;
}
