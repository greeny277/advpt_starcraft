// vim: ts=4:sw=4 expandtab
#include "Resources.h"

inline Resources::Resources(int gas, int minerals) :
    gas(gas),
    minerals(minerals) {
    }

inline void Resources::operator+=(Resources other) {
    gas += other.gas;
    minerals += other.minerals;
}
inline void Resources::operator-=(Resources other) {
    gas -= other.gas;
    minerals -= other.minerals;
}
inline Resources Resources::operator+(Resources other) const {
    return Resources(gas + other.gas, minerals + other.minerals);
}
inline Resources Resources::operator-(Resources other) const {
    return Resources(gas - other.gas, minerals - other.minerals);
}
inline Resources Resources::operator-() const {
    return Resources(-gas, -minerals);
}
inline Resources Resources::operator*(int factor) const {
    return Resources(gas * factor, minerals * factor);
}

inline bool Resources::allValuesLargerThan(Resources other) const {
    return gas > other.gas && minerals > other.minerals;
}
