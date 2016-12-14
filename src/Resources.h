// vim: ts=4:sw=4 expandtab
#pragma once

class Resources {
public:
    int gas;
    int minerals;

    inline Resources(int gas, int minerals) :
        gas(gas),
        minerals(minerals) {
    }

    inline void operator+=(Resources other) {
        gas += other.gas;
        minerals += other.minerals;
    }
    inline void operator-=(Resources other) {
        gas -= other.gas;
        minerals -= other.minerals;
    }
    inline Resources operator+(Resources other) const {
        return Resources(gas + other.gas, minerals + other.minerals);
    }
    inline Resources operator-(Resources other) const {
        return Resources(gas - other.gas, minerals - other.minerals);
    }
    inline Resources operator-() const {
        return Resources(-gas, -minerals);
    }
    inline Resources operator*(int factor) const {
        return Resources(gas * factor, minerals * factor);
    }

    inline bool allValuesLargerThan(Resources other) const {
        return gas > other.gas && minerals > other.minerals;
    }
};
