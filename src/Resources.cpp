// vim: ts=4:sw=4 expandtab
#include "Resources.h"

Resources::Resources(int gas, int minerals) :
    milliGas(gas * 1000),
    milliMinerals(minerals * 1000) {
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
    return Resources(milliGas + other.milliGas, milliMinerals + other.milliMinerals);
}
Resources Resources::operator-(Resources other) const {
    return Resources(milliGas - other.milliGas, milliMinerals - other.milliMinerals);
}
Resources Resources::operator-() const {
    return Resources(-milliGas, -milliMinerals);
}
Resources Resources::operator*(int factor) const {
    return Resources(milliGas * factor, milliMinerals * factor);
}

bool Resources::allValuesLargerThan(Resources other) const {
    return milliGas > other.milliGas && milliMinerals > other.milliMinerals;
}
