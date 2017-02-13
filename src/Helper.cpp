#include "Helper.h"
#include <algorithm>

bool buildOrderCheckOneOf(const std::vector<EntityBP*> &oneOf, const std::unordered_multiset<const EntityBP*> &dependencies) {
    if (!oneOf.empty()) {
        auto it = std::find_if(oneOf.begin(), oneOf.end(),
                [&](const EntityBP* req) {
                    return dependencies.find(req) != dependencies.end();
                });
        if (it == oneOf.end())
            return false;
    }
    return true;
}
