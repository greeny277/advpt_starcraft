#include "Helper.h"
#include <algorithm>

bool buildOrderCheckOneOf(const std::vector<std::string> &oneOf, const std::unordered_multiset<std::string> &dependencies) {
    if (!oneOf.empty()) {
        auto it = std::find_if(oneOf.begin(), oneOf.end(),
                [&](const std::string &req) {
                    return dependencies.find(req) != dependencies.end();
                });
        if (it == oneOf.end())
            return false;
    }
    return true;
}

bool isVespeneInst(const std::string &name) {
	if(name.compare("assimilator") == 0 || name.compare("refinery") == 0 || name.compare("extractor") == 0 ) {
		return true;
	}
	return false;
}
