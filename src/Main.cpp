// vim: ts=4:sw=4 expandtab
#include <iostream>
#include <stdlib.h>
#include <vector>
#include <typeinfo>
#include <unordered_map>
#include "EntityBP.h"

std::unordered_map<std::string, EntityBP> readConfig() {
    std::unordered_map<std::string, EntityBP> res;

    std::string line;
    std::string race;
    while(std::getline(std::cin, line)) {
        if (line[0] == '#') {
            continue;
        }

        //std::string name, type, minerals, vespene, buildTime, supplyCost, supplyProvided, startEnergy, maxEnergy, race, morphed_from, produced_by, dependency;
        std::string cells[13];
        std::stringstream lineStream(line);
        size_t i;
        for (i = 0; std::getline(lineStream, cells[i], ',') && i < 13; i++) {
        }
        if (i != 13) {
            std::cerr << "ignoring invalid line '" << line << "'" << std::endl;
            continue;
        }

        if (cells[1] == "building") {
            res.insert({cells[0], BuildingBP(cells)});
        } else {
            res.insert({cells[0], UnitBP(cells)});
        }


    }
    return res;
}

int main(int argc, char *argv[]) {
    std::unordered_map<std::string, EntityBP> blueprints = readConfig();

    while (true) {
    }

	return EXIT_SUCCESS;
}
