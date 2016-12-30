#pragma once
#include <unordered_set>

bool buildOrderCheckOneOf(const std::unordered_set<std::string> &oneOf, const std::unordered_multiset<std::string> &dependencies);
