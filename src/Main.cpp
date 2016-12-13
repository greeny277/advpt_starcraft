// vim: ts=4:sw=4 expandtab
#include <iostream>
#include <stdlib.h>
#include <vector>
#include <typeinfo>
#include <unordered_map>

class Entity {
    protected:
    std::string name;
    std::string race;

    int energy;
    int maxEnergy;
    Resources costs;
    int buildTime;
    std::vector<Ability> abilities;
    std::vector<std::string> requireOneOf; // we need one of the listed entities
    std::vector<std::string> producedByOneOf; // second required entity
    std::vector<std::string> morphedFrom; // third required entity. This entity is gone once this was built.
    int supplyProvided;

	// TODO Add Getter, add abilities for specific entities
}

class Unit : public Entity {
    int supplyCost;

    public:
        Unit(std::string data[13]) :
            name(data[0]),
            race(data[9]),
            energy(std::stoi(data[7])),
            maxEnergy(std::stoi(data[8])),
            costs(Resources(std::stoi(data[2]), std::stoi(data[3]))),
            buildTime(std::stoi(data[4])),
            abilities(),
            requiredEntities(),
            requireOneOf({data[12]}),
            producedByOneOf({data[11]}),
            morphedFrom(std::stoi(data[10])),
            supplyProvided(std::stoi(data[6])),

            supplyCost(std::stoi(data[5])) {

            std::stringstream requirementStream(data[12]);
            std::string req;
            while (std::getline(requirementStream, req, '/')) {
                requiredEntities.push_back(req);
            }
        }
}
class Building : public Entity {

    public:
        Building(std::string data[13]) :
            name(data[0]),
            race(data[9]),
            energy(std::stoi(data[7])),
            maxEnergy(std::stoi(data[8])),
            costs(Resources(std::stoi(data[2]), std::stoi(data[3]))),
            buildTime(std::stoi(data[4])),
            abilities(),
            requiredEntities(),
            requireOneOf({data[12]}),
            producedByOneOf({data[11]}),
            morphedFrom(std::stoi(data[10])),
            supplyProvided(std::stoi(data[6])) {

            std::stringstream requirementStream(data[12]);
            std::string req;
            while (std::getline(requirementStream, req, '/')) {
                requiredEntities.push_back(req);
            }
        }
}

std::unordered_map<std::string, Entity> readConfig() {
    std::unordered_map<std::string, Entity> res;

    std::string line;
    std::string race;
    while(std::getline(std::cin, line)) {
        if (line[0] == '#') {
            continue
        }

        //std::string name, type, minerals, vespene, buildTime, supplyCost, supplyProvided, startEnergy, maxEnergy, race, morphed_from, produced_by, dependency;
        std::string cells[13];
        std::stringstream lineStream(line);
        for (size_t i = 0; std::getline(lineStream, cells[i], ',') && i < 13; i++) {
        }
        if (i != 13) {
            std::cerr << "ignoring invalid line '" << line << "'" << std::endl;
            continue;
        }

        if (cells[1] == "building") {
            res.insert({cells[0], Building(cells)});
        } else {
            res.insert({cells[0], Unit(cells)});
        }


    }
    return result;
}

int main(int argc, char *argv[]) {
    std::map<std::string, std::typeinfo> 

    std::vector<Entity> buildOrder = readConfig();

    while (true) {
    }

	return EXIT_SUCCESS;
}
