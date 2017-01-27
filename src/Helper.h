#pragma once
#include <unordered_set>
#include <vector>

bool buildOrderCheckOneOf(const std::vector<std::string> &oneOf, const std::unordered_multiset<std::string> &dependencies);
bool isVespeneInst(const std::string &name);
