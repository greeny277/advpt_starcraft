#pragma once
#include <unordered_set>
#include <vector>
#include "EntityBP.h"

bool buildOrderCheckOneOf(const std::vector<EntityBP*> &oneOf, const std::unordered_multiset<const EntityBP*> &dependencies);
