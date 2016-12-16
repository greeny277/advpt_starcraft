// vim: ts=4:sw=4 expandtab
#include "Resources.h"

 Resources::Resources(int gas, int minerals) :
    gas(gas),
    minerals(minerals) {
    }

 void Resources::operator+=(Resources other) {
    gas += other.gas;
    minerals += other.minerals;
}
 void Resources::operator-=(Resources other) {
    gas -= other.gas;
    minerals -= other.minerals;
}
 Resources Resources::operator+(Resources other) const {
    return Resources(gas + other.gas, minerals + other.minerals);
}
 Resources Resources::operator-(Resources other) const {
    return Resources(gas - other.gas, minerals - other.minerals);
}
 Resources Resources::operator-() const {
    return Resources(-gas, -minerals);
}
 Resources Resources::operator*(int factor) const {
    return Resources(gas * factor, minerals * factor);
}

 bool Resources::allValuesLargerThan(Resources other) const {
    return gas > other.gas && minerals > other.minerals;
}
