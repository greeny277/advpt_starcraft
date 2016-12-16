// vim: ts=4:sw=4 expandtab
#pragma once

class Resources{
    public:
        int gas;
        int minerals;

        Resources(int gas, int minerals);

        void operator+=(Resources other);
        void operator-=(Resources other);
        Resources operator+(Resources other) const;
        Resources operator-(Resources other) const;
        Resources operator-() const;
        Resources operator*(int factor) const;
        bool allValuesLargerThan(Resources other) const;
};
