// vim: ts=4:sw=4 expandtab
#include "Resources.h"

Resources::Resources(int gas, int minerals, int divisor) :
    milliGas(gas * (1000 / divisor)),
    milliMinerals(minerals * (1000 / divisor)) {
}

void Resources::operator+=(Resources other) {
    milliGas += other.milliGas;
    milliMinerals += other.milliMinerals;
}
void Resources::operator-=(Resources other) {
    milliGas -= other.milliGas;
    milliMinerals -= other.milliMinerals;
}
Resources Resources::operator+(Resources other) const {
    return Resources(milliGas + other.milliGas, milliMinerals + other.milliMinerals, 1000);
}
Resources Resources::operator-(Resources other) const {
    return Resources(milliGas - other.milliGas, milliMinerals - other.milliMinerals, 1000);
}
Resources Resources::operator-() const {
    return Resources(-milliGas, -milliMinerals, 1000);
}
Resources Resources::operator*(int factor) const {
    return Resources(milliGas * factor, milliMinerals * factor, 1000);
}

bool Resources::allValuesLargerEquals(Resources other) const {
    return milliGas >= other.milliGas && milliMinerals >= other.milliMinerals;
}
