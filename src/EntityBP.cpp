// vim: ts=4:sw=4 expandtab
#include "EntityBP.h"
    inline EntityBP::EntityBP(std::string data[12]) :
        name(data[0]),
        race(data[8]),
        startEnergy(std::stoi(data[6])),
        maxEnergy(std::stoi(data[7])),
        costs(Resources(std::stoi(data[1]), std::stoi(data[2]))),
        buildTime(std::stoi(data[3])),
        abilities(),
        requireOneOf(),
        producedByOneOf({data[10]}),
        morphedFrom({data[9]}),
        supplyProvided(std::stoi(data[5])) {

        std::stringstream requirementStream(data[11]);
        std::string req;
        while (std::getline(requirementStream, req, '/')) {
            requireOneOf.push_back(req);
        }
    }
