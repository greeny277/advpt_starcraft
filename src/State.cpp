// vim: ts=4:sw=4 expandtab
#include "State.h"

 State::State(const std::string &race, const std::unordered_map<std::string, EntityBP> &blueprints) :
    currentSupply(0),
    resources{0, 50},
    currentMaxSupply(0),
    entities{},
    timestamp(0),
    runningActions{} {
        bool mainBuilding = false;
        bool workers = false;
        for (const auto &bp : blueprints) {
            if (bp.second.getRace() == race) {
                auto building = dynamic_cast<const BuildingBP*>(&bp.second);
                if (building != nullptr && !mainBuilding) {
                    entities.push_back(new BuildingInst(building));
                    mainBuilding = true;
                }
                auto unit = dynamic_cast<const UnitBP*>(&bp.second);
                if (unit != nullptr && !workers) {
                    for (size_t i = 0; i < 6; i++) {
                        entities.push_back(new WorkerInst(unit));
                    }
                    workers = true;
                }
            }
        }

        // TODO: 1 overlord, 3 larvae
    }