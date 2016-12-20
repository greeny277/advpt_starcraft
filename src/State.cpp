// vim: ts=4:sw=4 expandtab
#include "State.h"

State::State(const std::string &race, const std::unordered_map<std::string, EntityBP*> &blueprints_) :
    time(0),
    currentSupply(0),
    resources{0, 50},
    currentMaxSupply(0),
    entities{},
    timestamp(0),
    blueprints(blueprints_),
    alreadyProduced{},
    runningActions{} {
        bool mainBuilding = false;
        bool workers = false;
        for (const auto bp : blueprints) {
            if (bp.second->getRace() == race) {
                auto building = dynamic_cast<const BuildingBP*>(bp.second);
                if (building != nullptr && !mainBuilding) {
                    entities.push_back(building->newInstance());
                    mainBuilding = true;
                }
                auto unit = dynamic_cast<const UnitBP*>(bp.second);
                if (unit != nullptr && !workers) {
                    for (size_t i = 0; i < 6; i++) {
                        entities.push_back(unit->newInstance());
                    }
                    workers = true;
                }
            }
        }

        if (race == "zerg") {
            auto larva = static_cast<const UnitBP*>(blueprints.at("larva"));
            for (size_t i = 0; i < 3; i++) {
                entities.push_back(larva->newInstance());
            }
            entities.push_back(blueprints.at("overlord")->newInstance());
        }
}
nlohmann::json State::getUnitJSON() const {
    nlohmann::json units;
    for (const EntityInst *inst : entities) {
        auto name = inst->getBlueprint()->getName();
        if (units.find(name) == std::end(units)) {
            units[name] = nlohmann::json::array();
        }
        units[name].push_back(inst->getID());
    }
    return units;
}

std::vector<EntityInst*>& State::getEntities(){
    return entities;
}

void State::addEntityInst(EntityInst *e) {
    alreadyProduced.insert(e->getBlueprint()->getName());
    entities.push_back(e);
}
