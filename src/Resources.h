// vim: ts=4:sw=4 expandtab
#pragma once

class Resources{
    private:
        int milliGas;
        int milliMinerals;

    public:
        inline int getGas() const {
            return milliGas / 1000;
        }
        inline int getMinerals() const {
            return milliMinerals / 1000;
        }
        inline void setGas(int val) {
            milliGas = val * 1000;
        }
        inline void setMinerals(int val) {
            milliMinerals = val * 1000;
        }

        Resources(int gas, int minerals, int divisor=1);

        void operator+=(Resources other);
        void operator-=(Resources other);
        Resources operator+(Resources other) const;
        Resources operator-(Resources other) const;
        Resources operator-() const;
        Resources operator*(int factor) const;
        bool allValuesLargerThan(Resources other) const;
};
